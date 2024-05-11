#ifndef AUDIOWAVE
#define AUDIOWAVE

#include <windows.h>


struct audio_state
{
	int sample_rate;
	double dt;
	int bytes_per_sample;
	int freq;
	int running_sample_index;
	int wave_counter;
};

struct rect_state 
{
	int x;
	int y;
	int width;
	int height;
};

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


void render_rect(struct window_buffer *b, int top, int left, int width, int height);

struct window_buffer getMainWindowBuffer();

void set_colour(unsigned char red, unsigned char green, unsigned char blue);

void display_buffer(HDC handle_device_context, struct window_buffer *b);
#endif
