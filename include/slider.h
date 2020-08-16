#pragma once

#include <raylib.h>

typedef struct {
	int x;
	int y;
	int width;
	float min_value;
	float max_value;
	float* value;
	
} Slider;

Slider* Slider_create(int x, int y, int width, float* value, float min_value, float max_value);
void Slider_destroy(Slider* slider);

void Slider_update();
void Slider_draw(Slider* slider);
