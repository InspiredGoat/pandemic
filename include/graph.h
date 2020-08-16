#pragma once
#include <raylib.h>

#include "types.h"

typedef struct {
	uint max_points;
	uint current_point;
	float* data_points;
} Graph;

Graph* Graph_create(uint max_points);
void Graph_destroy(Graph* graph);

float Graph_get_highest_value(Graph* graph);

void Graph_add_point(Graph* graph, float value);
void Graph_draw(Graph* graph, int x, int y, int width, int height, float max_value, uint show_up_to, float line_thickness, Color line_color);
