#include <stdlib.h>

#include "../include/graph.h"

Graph* Graph_create(uint max_points) {
	Graph* graph = (Graph*) malloc(sizeof(Graph));	
	graph->max_points = max_points;
	graph->data_points = (float*) malloc(sizeof(float) * max_points);
	graph->current_point = 0;
	return graph;
}

void Graph_destroy(Graph* graph) {
	if(!graph->data_points)
		free(graph->data_points);
	if(!graph)
		free(graph);
}


float Graph_get_highest_value(Graph* graph) {
	float highest_value = graph->data_points[0];

	for(uint i = 0; i < graph->max_points; i++)
		if(highest_value < graph->data_points[i])
			highest_value = graph->data_points[i];

	return highest_value;
}

void Graph_add_point(Graph* graph, float value) {
	uint current_point = graph->current_point;
	uint max_points = graph->max_points;

	if(current_point < max_points) {
		graph->data_points[current_point] = value;
		graph->current_point++;
	}
	else {
		for(uint i = 0;i < graph->max_points-1; i++) {
			graph->data_points[i] = graph->data_points[i+1];
		}
		graph->data_points[graph->max_points-1] = value;
	}
}

void Graph_draw(Graph* graph, int x, int y, int width, int height, float max_value, uint show_up_to, float line_thickness, Color line_color) {
	uint current_point = graph->current_point;
	uint max_points = graph->max_points;

	if(current_point == 0)
		return;

// (current_point > show_up_to) * (current_point - show_up_to)

	for(uint i = 0; i < current_point - 1; i++) {
//		i = i % (show_up_to-1);
//		max_points = max_points % show_up_to;

		int point_x = x + (int) ((float) i * (width/(float)max_points));
		int neighbour_x = x + (int) ((float) (i + 1) * (width/(float)max_points));

		int point_y = y + (int) (graph->data_points[i] * (height/max_value));
		int neighbour_y = y + (int) (graph->data_points[i + 1] * (height/max_value));

		DrawLineEx((Vector2) { point_x, point_y }, (Vector2) { neighbour_x, neighbour_y }, line_thickness, line_color);
	}
}
