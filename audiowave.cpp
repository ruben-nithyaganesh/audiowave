#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <stdio.h>

struct window_buffer
{
  BITMAPINFO bitmap_info;
  void* data;
  int bytes_per_pixel;
  int pitch;
  int width;
  int height;
};

struct window_dimension
{
	int top;
	int left;
	int width;
	int height;
};

struct rgb {
 unsigned char red;
 unsigned char green;
 unsigned char blue;
};

static struct rgb c_state;
static boolean running;

RECT window_rect;
static struct window_buffer window_buffer;
static struct window_dimension window_dim;


void set_colour(unsigned char red, unsigned char green, unsigned char blue)
{
	c_state.red = red;
	c_state.blue = blue;
	c_state.green = green;
}

void resize_buffer(struct window_buffer *wb, int width, int height)
{
	if(wb->data)
	{
		VirtualFree(wb->data, 0, MEM_RELEASE);
	}

	wb->width = width;
	wb->height = height;
 
	wb->bytes_per_pixel = 4;
	wb->bitmap_info.bmiHeader.biSize = sizeof(wb->bitmap_info.bmiHeader);
	wb->bitmap_info.bmiHeader.biWidth	= wb->width;
	wb->bitmap_info.bmiHeader.biHeight = wb->height; // TODO: is this right?
	wb->bitmap_info.bmiHeader.biPlanes = 1;
	wb->bitmap_info.bmiHeader.biBitCount = 32;
	wb->bitmap_info.bmiHeader.biCompression = BI_RGB;

	int bitmap_mem_size = (wb->width * wb->height) * wb->bytes_per_pixel;
	wb->data = VirtualAlloc(0, bitmap_mem_size, MEM_COMMIT, PAGE_READWRITE);

	wb->pitch = width*wb->bytes_per_pixel;
}

void window_resized(HWND hwnd)
{
	GetWindowRect(hwnd, &window_rect);
	int w_width = window_rect.right - window_rect.left;
	int w_height = window_rect.bottom - window_rect.top;
	char output_msg[100];
	sprintf(output_msg, "width: %d\nheight: %d\n", w_width, w_height);
	OutputDebugStringA(output_msg);

	window_dim.top = window_rect.top;
	window_dim.left = window_rect.left;
	window_dim.width = w_width;
	window_dim.height = w_height;
}

void display_buffer(HDC handle_device_context, struct window_buffer *b)
{
	StretchDIBits(
			handle_device_context,
			0, //xDest
			0, //yDest
			window_dim.width, //DestWidth
			window_dim.height, //DestHeight
			0, //xSrc
			0, //ySrc
			b->width, //SrcWidth
			b->height, //SrcHeight
			b->data, //lpBits (data to render)
			&b->bitmap_info, //lpbmi
			DIB_RGB_COLORS, //iUsage
			SRCCOPY //directly copy source to dest. see rop in https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-bitblt
		);
}

void render_colour(struct window_buffer *b)
{
	int bytes_per_pixel = 4;
	int width = b->width;
	int height = b->height;
	int pitch = b->pitch;

	unsigned char *row = (unsigned char *)b->data;
	
	for(int y = 0; y < height; y++)
	{
		unsigned char *pixel = row;
		for(int x = 0; x < width; x++)
		{
			*pixel = c_state.blue; pixel++;
			*pixel = c_state.green; pixel++;
			*pixel = c_state.red; pixel++;
			*pixel = 0; pixel++;
		}
		row += pitch;
	}
}

void render_rect(struct window_buffer *b, int top, int left, int width, int height)
{
	int bytes_per_pixel = 4;
	int b_width = b->width;
	int pitch = bytes_per_pixel * b_width;

	unsigned char *row = (unsigned char *)(b->data);
	row += (pitch*(top)) + left;
	for(int y = 0; y < height; y++)
	{
		unsigned char *pixel = row;
		for(int x = 0; x < width; x++)
		{
			*pixel = c_state.blue; pixel++;
			*pixel = c_state.green; pixel++;
			*pixel = c_state.red; pixel++;
			*pixel = 0; pixel++;
		}
		row += pitch;
	}
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_DESTROY:
		{
				PostQuitMessage(0);
				return 0;
		}
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			window_resized(hwnd);
			render_colour(&window_buffer);
			set_colour(0, 0, 0);
			render_rect(&window_buffer, 100, 200, 500, 250);
			display_buffer(hdc, &window_buffer);
			EndPaint(hwnd, &ps);
		}
		case WM_SIZE:
		{
			window_resized(hwnd);
			resize_buffer(&window_buffer, window_dim.width, window_dim.height);
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{

	//Register the class 
	const wchar_t CLASS_NAME[] = L"Audiowave Window Class";

	WNDCLASS wc = {};

	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);


	//Lets create a window
	
	HWND hwnd = CreateWindowEx(
		0, //optional window styles
		CLASS_NAME, //class name
		L"Audiowave", //window name
		WS_OVERLAPPEDWINDOW, //dwStyle
		
		//default size and position
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

		NULL, //parent window
		NULL, //menu
		hInstance, //Instance handle
		NULL //additional data
	);

	if(!hwnd)
	{
		printf("Problem getting window handle hwnd");
		return -1;
	}

	set_colour(200, 100, 200);
	running = true;

	ShowWindow(hwnd, nCmdShow);

	window_resized(hwnd);
	resize_buffer(&window_buffer, window_dim.width, window_dim.height);
	//do the msg loop
	MSG msg = {};
	
	while(GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
