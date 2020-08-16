#include <stdlib.h>

#include "../include/slider.h"

Slider* Slider_create(int x, int y, int width, int height, float* value, float min_value, float max_value) {
	Slider* slider = (Slider*) malloc(sizeof(Slider));
	slider->x = x;
	slider->y = y;
	slider->width = width;
	slider->height = height;

	// Find the ratio according to the current value
	slider->ratio = (*value - min_value) / (max_value - min_value);
	slider->value = value;
	slider->min_value = min_value;
	slider->max_value = max_value;
	slider->hovered = false;
	return slider;
}

void Slider_destroy(Slider* slider) {
	free(slider);
}


void Slider_update(Slider* slider) {
	if(slider->hovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
		slider->ratio = ((float) (GetMouseX() - slider->x) / (float) slider->width);
		if(slider->ratio > 1)
			slider->ratio = 1;
		if(slider->ratio < 0)
			slider->ratio = 0;

		*slider->value = slider->ratio * (slider->max_value - slider->min_value) + slider->min_value;
	}

	if((GetMouseX() > slider->x-15 && GetMouseX() < slider->x + slider->width+15) && (GetMouseY() > slider->y-15 && GetMouseY() < slider->y + slider->height+15))
		slider->hovered = true;
	else
		slider->hovered = false;
}

void Slider_draw(Slider* slider, Color filled, Color empty) {
	int x_offset = (int) ((float)slider->width * slider->ratio);
	DrawRectangle(slider->x, slider->y, slider->width, slider->height, empty);
	DrawRectangle(slider->x, slider->y, x_offset, slider->height, filled);
	DrawCircle(x_offset+15, slider->y +1 , 5.f, filled);
}
