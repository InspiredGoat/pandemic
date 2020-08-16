#include "../include/slider"

Slider* Slider_create(int x, int y, int width, float* value, float min_value, float max_value) {
	Slider* slider = (Slider*) malloc(sizeof(Slider));
	slider->x = x;
	slider->y = y;
	slider->width = width;
	slider->value = value;
	slider->min_value = min_value;
	slider->max_value = max_value;
	return slider;
}

void Slider_destroy(Slider* slider) {
	free(slider);
}


void Slider_update(Slider* slider) {
	
}

void Slider_draw(Slider* slider) {
	DrawRectangle(slider->x, slider->y, );
}
