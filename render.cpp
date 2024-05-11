#include "audiowave.h"


void render_grid(struct window_buffer wb, int top, int left)
{
	int cell_size = 100;

	int rows = 10;
	int cols = 10;

	int gap = 10;
	

	set_colour(200, 200, 200);

	for(int row = 0; row < rows; row++) {
		for(int col = 0; col < cols; col++) {
			

			int cell_top = row * (cell_size + gap) + top;
			int cell_left = col * (cell_size + gap) + left;

			render_rect(&wb, cell_top, cell_left, cell_size, cell_size);

		}
	}
}

void render(struct window_buffer wb) {

	render_grid(wb, 100, 500);
	render_grid(wb, 59, 249);
}
