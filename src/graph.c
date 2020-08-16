#include <stdlib.h>

#include "../include/graph.h"

Graph* Graph_create(uint max_points) {
	Graph* graph = (Graph*) malloc(sizeof(Graph));	
	graph->max_points = max_points;
	graph->data_points = (float*) malloc(sizeof(float) * max_points);
	return graph;
}

void Graph_destroy(Graph* graph) {
	if(!graph->data_points)
		free(graph->data_points);
	if(!graph)
		free(graph);
}

void Graph_add_point(Graph* graph, float value) {
	uint current_point = graph->current_point;
	uint max_points = graph->max_points;

	graph->data_points[current_point] = value;
	graph->current_point = (current_point+1)%max_points;
//	if(current_point < max_points) {
//		graph->data_points[current_point] = value;
//		current_point++;
//	}
//	else {
//		for(uint i = graph->max_points-1; i > 0; i--) {
//			graph->data_points[i] = graph->data_points[i+1];
//		}
//		graph->data_points[graph->max_points-1] = value;
//	}
}

void Graph_draw(Graph* graph, int x, int y, int width, int height, float max_value, float line_thickness, Color line_color) {
	for(uint i = 0; i < graph->max_points-1; i++) {
		int point_x = x + (int) ((float) i * (width/(float)graph->max_points));
		int neighbour_x = x + (int) ((float) (i + 1) * (width/(float)graph->max_points));

		int point_y = y + (int) (graph->data_points[i] * (height/max_value));
		int neighbour_y = y + (int) (graph->data_points[i + 1] * (height/max_value));

		DrawLineEx((Vector2) { point_x, point_y }, (Vector2) { neighbour_x, neighbour_y }, line_thickness, line_color);
	}
}
