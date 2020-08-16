#include <stdio.h>
#include <stdlib.h>

#include <raylib.h>

#include "../include/types.h"
#include "../include/graph.h"

typedef struct {
	Vector2* positions;
	Vector2* directions;
	float* square_distances;
	byte* infected_periods;
	bool* simulate;

	ushort count;
	uint square_distance_count;
} Population;


// Global screen variables
Vector2 mouse_pos_prev;

// Global world variables
ushort g_world_width;
ushort g_world_height;

Vector2* g_hotspots;
ushort g_hotspot_count; 

Rectangle g_sections[30];
ushort g_section_count = 0;


// Helper functions

inline float randf() {
	return (rand()%10000) / 10000.f;
}

void rand_vector_array(Vector2* v, uint size, float min, float max) {
	for(uint i = 0; i < size; i++) {
		v[i].x = (randf() * (max - min)) + min;
		v[i].y = (randf() * (max - min)) + min;
	}
}

int min(int x, int y) {
	return (x < y) * x + (x > y) * y;
}

int max(int x, int y) {
	return (x < y) * y + (x > y) * x;
}


// World functions

bool section_is_valid(Rectangle section) {
	for(ushort i = 0; i < g_section_count; i++) {
		if(CheckCollisionRecs(section, g_sections[i]))
			return true;
	}

	return false;
}

void section_add(Rectangle section) {
	
}


// Player functions

void player_move(Camera2D* camera, float delta) {
	Vector2 mouse_pos;
	mouse_pos.x = GetMouseX();
	mouse_pos.y = GetMouseY();

	if(IsMouseButtonDown(0)) {
		camera->target.x -= (mouse_pos.x - mouse_pos_prev.x) / camera->zoom;
		camera->target.y -= (mouse_pos.y - mouse_pos_prev.y) / camera->zoom;
	}

	camera->offset.x = GetScreenWidth() / 2;
	camera->offset.y = GetScreenHeight()  / 2;

	int zoom_delta = (((-3 * GetMouseWheelMove()) | -IsKeyDown(KEY_Z)) | IsKeyDown(KEY_X));
	camera->zoom *= 1 - zoom_delta * 4.9715f * delta;

	mouse_pos_prev = mouse_pos;
}



// Population functions

Population* Population_create(ushort agent_count) {
	Population* population = (Population*) malloc(sizeof(Population));
	population->count = agent_count;

	population->positions = (Vector2*) malloc(sizeof(Vector2) * agent_count);
	population->directions = (Vector2*) malloc(sizeof(Vector2) * agent_count);
	population->infected_periods = (byte*) malloc(sizeof(byte) * agent_count);

	uint n = (uint) (agent_count-1);
	n = n * n + n;
	population->square_distance_count = n;
	population->square_distances = (float*) malloc(sizeof(float) * n);

	return population;
}

void Population_destroy(Population* population) {
	if(population->positions != NULL)
		free(population->positions);

	if(population->directions != NULL)
		free(population->directions);

	if(population->square_distances != NULL)
		free(population->square_distances);

	if(population->infected_periods != NULL)
		free(population->infected_periods);

	if(population != NULL)
		free(population);
}

void agents_find_distances(float* square_distances, Vector2* positions, ushort agent_count) {
	for(ushort i = 0; i < agent_count; i++) {
	}
}

void agents_steer(Vector2* directions, Vector2* positions, ushort agent_count) {
	for(ushort i = 0; i < agent_count; i++) {
		
	}
}

void agents_move(Vector2* directions, Vector2* positions, ushort agent_count, float delta) {
	for(ushort i = 0; i < agent_count; i++) {
		positions[i].x += directions[i].x * delta * 50.f;
		positions[i].y += directions[i].y * delta * 50.f; 
	}
}

void agents_spread_disease(Vector2* positions, byte* infected_periods, ushort agent_count) {
	for(ushort i = 0; i < agent_count; i++) {
		if(infected_periods[i] > 0) {
			for(ushort j = 0; j < agent_count; j++) {
				
			}
		}
	}
}

void agents_draw(Vector2* positions, ushort agent_count) {
	for(ushort i = 0; i < agent_count; i++) {
		DrawCircle(positions[i].x, positions[i].y, 5, WHITE);
	}
}


int main() {
	InitWindow(800, 800, "Pandemic");

	Population* population = Population_create(100);

	Vector2* positions = population->positions;
	Vector2* directions = population->directions;
	byte* infected_periods = population->infected_periods;
	ushort agent_count = population->count;

	Camera2D camera = { 0 };
	camera.zoom = 1.f;

	g_sections[0].x = 0;
	g_sections[0].y = 0;
	g_sections[0].width = 1000;
	g_sections[0].height = 1000;

	g_section_count = 1;

	float delta = 0;

	Graph* graph = Graph_create(1000);

	rand_vector_array(positions, agent_count, 0, g_sections[0].width);
	rand_vector_array(directions, agent_count, -1, 1);

	float counter = 0;

	while(!WindowShouldClose()) {
		delta = GetFrameTime();

		counter += delta;

		Graph_add_point(graph, GetFPS());

		// Player input
		player_move(&camera, delta);

		// Simulation
		agents_steer(directions, positions, agent_count);
		agents_move(directions, positions, agent_count, delta);
		agents_spread_disease(positions, infected_periods, agent_count);
		
		// Rendering
		BeginDrawing();
		ClearBackground(BLACK);

		// Draw scene
		BeginMode2D(camera);

		agents_draw(positions, agent_count);
		for(ushort i = 0; i < g_section_count; i++) {
			DrawRectangleLinesEx(g_sections[i], 5, GREEN);
		}
		EndMode2D();

		// Draw UI
		DrawFPS(0, 0);
		DrawRectangle(0, 0, 400, 400, GREEN);
		float max = Graph_get_highest_value(graph);
		Graph_draw(graph, 0, 0, 400, 400, max + 10.f, 100, 3.f, RED);
		EndDrawing();
	}

	Graph_destroy(graph);
	Population_destroy(population);
	return 0;
}
