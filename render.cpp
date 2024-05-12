#include "audiowave.h"

#define GRID_WIDTH 28
#define GRID_HEIGHT 28
#define GRID_TOP 10
#define GRID_LEFT 10
#define GRID_GAP 5
#define GRID_CELL_SIZE 25

static int mouseX = 0;
static int mouseY = 0;

bool selected[GRID_HEIGHT][GRID_WIDTH];
int hoveredY, hoveredX;

void select()
{
	selected[hoveredY][hoveredX] = !selected[hoveredY][hoveredX];
}

void clear()
{
	int rows = GRID_HEIGHT;
	int cols = GRID_WIDTH;
	for(int row = 0; row < rows; row++) {
		for(int col = 0; col < cols; col++) {
			selected[col][row] = false;	
		}
	}
}

void mouse_input(int yPos, int xPos)
{
	int cell_size = GRID_CELL_SIZE;

	int rows = GRID_HEIGHT;
	int cols = GRID_WIDTH;

	int gap = GRID_GAP;
	mouseX = xPos;
	mouseY = yPos;
	for(int row = 0; row < rows; row++) {
		for(int col = 0; col < cols; col++) {
			

			int cell_top = row * (cell_size + gap) + GRID_TOP;
			int cell_left = col * (cell_size + gap) + GRID_LEFT;
			
			if(cell_top <= mouseY && (cell_top + GRID_CELL_SIZE) >= mouseY
					&& cell_left <= mouseX && (cell_left + GRID_CELL_SIZE) >= mouseX)
			{
				hoveredY = col;
				hoveredX = row;

				return;
			}
		}
	}
}

void render_grid(struct window_buffer wb, int top, int left)
{
	int cell_size = GRID_CELL_SIZE;

	int rows = GRID_HEIGHT;
	int cols = GRID_WIDTH;

	int gap = GRID_GAP;
	

	set_colour(200, 200, 200);

	for(int row = 0; row < rows; row++) {
		for(int col = 0; col < cols; col++) {
			

			int cell_top = row * (cell_size + gap) + top;
			int cell_left = col * (cell_size + gap) + left;
			
			if((col == hoveredY && row == hoveredX) || selected[col][row])
			{
				set_colour(100, 100, 100);
			}
			else
			{
				set_colour(200, 200, 200);
			}
			render_rect(&wb, cell_top, cell_left, cell_size, cell_size);

		}
	}
}

void render(struct window_buffer wb) {
	render_grid(wb, GRID_TOP, GRID_LEFT);
}
