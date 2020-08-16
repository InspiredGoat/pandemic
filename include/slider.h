#pragma once

#include <raylib.h>

typedef struct {
	int x;
	int y;
	int width;
	int height;

	float min_value;
	float max_value;
	float ratio;
	float* value;
	
	bool hovered;
} Slider;

Slider* Slider_create(int x, int y, int width, int height, float* value, float min_value, float max_value);
void Slider_destroy(Slider* slider);

void Slider_update(Slider* slider);
void Slider_draw(Slider* slider, Color filled, Color empty);
