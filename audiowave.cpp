#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <stdio.h>
#include <dsound.h>
#include <stdint.h>
#include <math.h>
#include <winuser.h>
#include "audiowave.h"
#include "render.h"


#define PI 3.14159265359

#define WIN_APP_BANNER_HEIGHT 32

typedef uint16_t uint16;
typedef uint32_t uint32;

static int ToneVolume = 500;
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);



static struct rgb c_state;
static boolean running;

RECT window_rect;
static struct window_buffer window_buffer;
static struct window_dimension window_dim;
static struct rect_state rect_state;
static struct audio_state audio_state;
static int move_speed = 10;
static LPDIRECTSOUNDBUFFER secondary_buffer;

//Sound test
static int SamplesPerSecond = 48000;
static int ToneHz = 256;
static uint32 RunningSampleIndex = 0;
static int SquareWaveCounter = 0;
static int SquareWavePeriod = SamplesPerSecond/ToneHz;
static int HalfSquareWavePeriod = SquareWavePeriod/2;
static int BytesPerSample = sizeof(uint16) * 2;
static int SecondaryBufferSize = SamplesPerSecond*BytesPerSample;
static double dt = 1.0 / SamplesPerSecond;

void set_colour(unsigned char red, unsigned char green, unsigned char blue)
{
	c_state.red = red;
	c_state.blue = blue;
	c_state.green = green;
}

void init_DirectSound(HWND Window, unsigned int SamplesPerSecond, unsigned int BufferSize) 
{

	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

	if(!DSoundLibrary)
	{
		OutputDebugStringA("DSoundLibrary failed");
		return;
	}

	direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

	LPDIRECTSOUND DirectSound;

	if(!DirectSoundCreate || !SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
	{
		OutputDebugStringA("DirectSoundCreate failed\n");
		return;
	}

	WAVEFORMATEX WaveFormat 	= {};
	WaveFormat.wFormatTag 		= WAVE_FORMAT_PCM;
	WaveFormat.nChannels 		= 2;
	WaveFormat.wBitsPerSample 	= 16;
	WaveFormat.nSamplesPerSec 	= SamplesPerSecond;
	WaveFormat.nBlockAlign 		= (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
	WaveFormat.nAvgBytesPerSec 	= WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
	WaveFormat.cbSize 			= 0;

	if(!SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
	{
		OutputDebugStringA("SetCooperativeLevel failed\n");
		return;
	}

	LPDIRECTSOUNDBUFFER PrimaryBuffer;
	DSBUFFERDESC BufferDescription 	= {};
	BufferDescription.dwFlags 		= DSBCAPS_PRIMARYBUFFER;
	BufferDescription.dwSize		= sizeof(BufferDescription);

	if(!SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
	{
		OutputDebugStringA("CreateSoundBuffer failed\n");
		return;
	}

	if(!SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
	{
		OutputDebugStringA("SetFormat failed\n");
		return;
	}

	OutputDebugStringA("YAY: Initialised Primary Buffer\n");

	BufferDescription = {};
	BufferDescription.dwSize		= sizeof(BufferDescription);
	BufferDescription.dwFlags 		= 0;
	BufferDescription.dwBufferBytes = BufferSize; BufferDescription.lpwfxFormat 	= &WaveFormat;

	if(!SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &secondary_buffer, 0))) {
		OutputDebugStringA("CreateSoundBuffer for secondary failed\n");
		return;
	}

	OutputDebugStringA("YAY: Initialised Secondary Buffer\n");
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
	//char output_msg[100];
//	sprintf(output_msg, "width: %d\nheight: %d\n", w_width, w_height);
//	OutputDebugStringA(output_msg);

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

//render the current set colour over the entire supplied window buffer
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
	int b_height = b->height;
	int pitch = bytes_per_pixel * b_width;
	
	if(top < 0)
	{
		top = 0;
	}
	if(left < 0)
	{
		left = 0;
	}
	if((left+width) > b_width)
	{
		width = b_width - left;
	}
	if((top+height) > b_height)
	{
		height = b_height - top;
	}
	unsigned char *row = (unsigned char *)(b->data);
	row += (pitch*(b_height - top - height)) + (left * bytes_per_pixel);
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

	LRESULT result = 0;

	switch (uMsg)
	{
		case WM_DESTROY: case WM_QUIT: case WM_CLOSE:
		{
				PostQuitMessage(0);
				running = false;
				return 0;
		}
		case WM_PAINT:
		{
//			PAINTSTRUCT ps;
//			HDC hdc = BeginPaint(hwnd, &ps);
//			window_resized(hwnd);
//			render_colour(&window_buffer);
//			set_colour(0, 0, 0);
//			render_rect(&window_buffer, 100, 100, 300, 100);
//			display_buffer(hdc, &window_buffer);
//			EndPaint(hwnd, &ps);
		}
		case WM_SIZE:
		{
			window_resized(hwnd);
			resize_buffer(&window_buffer, window_dim.width, window_dim.height);
			render_colour(&window_buffer);
		}
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			int VKCode = wParam;
			bool was_down = ((wParam & (1 << 30)) != 0);
			bool is_down = ((wParam & (1 << 31)) == 0);
			if(is_down && !was_down){
				
				if(VKCode == 'W')
				{
					rect_state.y -= move_speed;
					if(rect_state.y < 0)
					{
						rect_state.y = 0;
					}
				}
				else if(VKCode == 'A')
				{
					rect_state.x -= move_speed;
					if(rect_state.x < 0)
					{
						rect_state.x = 0;
					}
				}
				else if(VKCode == 'S')
				{
					select();
					rect_state.y += move_speed;
					if(rect_state.y + rect_state.height > window_dim.height)
					{
						rect_state.y = window_dim.height - rect_state.height;
					}
				}
				else if(VKCode == 'D')
				{
					rect_state.x += move_speed;
					if(rect_state.x + rect_state.width > window_dim.width)
					{
						rect_state.x = window_dim.width - rect_state.width;
					}
				}
				else if(VKCode == 'Q')
				{
					PostQuitMessage(0);
					running = false;
					return 0;
				}
				else if(VKCode == 'E')
				{
					
				}
				else if(VKCode == 'C')
				{
					clear();
					
				}
				else if(VKCode == VK_UP)
				{
					
				}
				else if(VKCode == VK_LEFT)
				{
					
				}
				else if(VKCode == VK_DOWN)
				{
					
				}
				else if(VKCode == VK_RIGHT)
				{
					
				}
				else if(VKCode == VK_ESCAPE)
				{
					
				}
				else if(VKCode == VK_SPACE)
				{
					
				}
			}
		}
		default:
		{
			result = DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
	}
	return result;
}

void play_sound()
{
	
	DWORD PlayCursor;
	DWORD WriteCursor;
	if(SUCCEEDED(secondary_buffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
	{
		DWORD ByteToLock = (RunningSampleIndex * BytesPerSample) % SecondaryBufferSize;
		DWORD BytesToWrite;
		if(ByteToLock > PlayCursor)
		{
			BytesToWrite = SecondaryBufferSize - ByteToLock;
			BytesToWrite += PlayCursor;
		}
		else 
		{
			BytesToWrite = PlayCursor - ByteToLock;
		}
		VOID *Region1;
		DWORD Region1Size;
		VOID *Region2;
		DWORD Region2Size;

		if(SUCCEEDED(secondary_buffer->Lock(
					ByteToLock,
					BytesToWrite,
					&Region1, &Region1Size,
					&Region2, &Region2Size,
					0
		)))
		{
			uint16 *SampleOut = (uint16*)Region1;
			DWORD Region1SampleCount = Region1Size / BytesPerSample;
			for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; SampleIndex++)
			{
				if(SquareWaveCounter == 0)
				{
					SquareWaveCounter = SquareWavePeriod;
				}
				uint16 SampleValue = ToneVolume * (sin(((PI * 2.0) * 261.63) * ((double)RunningSampleIndex * dt))
															+ sin(((PI * 2.0) * 329.628) * ((double)RunningSampleIndex * dt))
															+ sin(((PI * 2.0) * 392.0) * ((double)RunningSampleIndex * dt)));
				*SampleOut++ = SampleValue;
				*SampleOut++ = SampleValue;
				RunningSampleIndex++;
			}
			SampleOut = (uint16*)Region2;
			DWORD Region2SampleCount = Region2Size / BytesPerSample;
			for(DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; SampleIndex++)
			{
				if(SquareWaveCounter == 0)
				{
					SquareWaveCounter = SquareWavePeriod;
				}
				uint16 SampleValue = ToneVolume * (sin(((PI * 2.0) * 261.63) * ((double)RunningSampleIndex * dt))
															+ sin(((PI * 2.0) * 329.628) * ((double)RunningSampleIndex * dt))
															+ sin(((PI * 2.0) * 392.0) * ((double)RunningSampleIndex * dt)));
				*SampleOut++ = SampleValue;
				*SampleOut++ = SampleValue;
				RunningSampleIndex++;
			}
		}

		secondary_buffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

struct window_buffer getMainWindowBuffer()
{
	return window_buffer;
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

	audio_state.sample_rate = 48000;
	audio_state.dt = 1.0 / (double)audio_state.sample_rate;
	audio_state.bytes_per_sample = 2 * sizeof(uint16);
	audio_state.freq = 256;
	audio_state.wave_counter = 0;
	

	rect_state.x = 100;
	rect_state.y = 100;
	rect_state.width = 100;
	rect_state.height = 100;

	set_colour(200, 100, 200);
	running = true;

	ShowWindow(hwnd, nCmdShow);

	HDC hdc = GetDC(hwnd);
	window_resized(hwnd);
	resize_buffer(&window_buffer, window_dim.width, window_dim.height);
	//do the msg loop
	MSG msg = {};
	
	init_DirectSound(hwnd, 48000, 48000*sizeof(uint16)*2);
	secondary_buffer->Play(0, 0, DSBPLAY_LOOPING);
	while(running)
	{
		MSG msg;
		while(PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		

		POINT point;
		GetCursorPos(&point);
		
		RECT rect;
		GetWindowRect(hwnd, &rect);

		int mouseWindowPosY = point.y - rect.top - WIN_APP_BANNER_HEIGHT;
		int mouseWindowPosX = point.x - rect.left;

		char output_msg[100];
		sprintf(output_msg, "x: %d\ny: %d\n", mouseWindowPosX, mouseWindowPosY);
		OutputDebugStringA(output_msg);

		mouse_input(mouseWindowPosY, mouseWindowPosX);

		window_resized(hwnd);
		char grayscale = 225;
		set_colour(grayscale, grayscale, grayscale);
		render_colour(&window_buffer);
		set_colour(178, 206, 254);
		//render_rect(&window_buffer, rect_state.y, rect_state.x, rect_state.width, rect_state.height);
		//play_sound();

		render(window_buffer);
		display_buffer(hdc, &window_buffer);
	}

	return 0;
}
