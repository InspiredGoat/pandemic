#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <raylib.h>
#include <raymath.h>

#include "../include/types.h"
#include "../include/graph.h"
#include "../include/slider.h"

typedef struct {
	Vector2* positions;
	Vector2* directions;
	float* square_distances;
	byte* infected_periods;
	byte* time_till_death;
	bool* simulated;

	ushort count;
	uint square_distance_count;
} Population;


//----------------------------------------------------------------------------------------------------------------------------------


// Global screen variables
Vector2 mouse_pos_prev;

// Global world variables
ushort g_world_width;
ushort g_world_height;

Vector2* g_hotspots;
ushort g_hotspot_count; 

Rectangle g_sections[30];
ushort g_section_count = 0;

// Population parameters
float g_social_distance = 20;
float g_social_distance_factor = .5f;

// Disease parameters
float g_infection_radius = 42;
float g_infection_chance = 0.2f;
float g_infection_duration = 10;

// Colors
Color ui_dark_grey = (Color) { 32, 32, 34, 255 };
Color ui_light_grey = (Color) { 60, 60, 66, 255 };


//----------------------------------------------------------------------------------------------------------------------------------


// Helper functions

inline float square_root(float number) { 
	int i; 
	float x, y; 
	x = number * 0.5; 
	y = number; 
	i = * (int *) &y; 
	i = 0x5f3759df - (i >> 1); 
	y = * (float *) &i; 
	y = y * (1.5 - (x * y * y)); 
	y = y * (1.5 - (x * y * y)); 
	return number * y; 
}

inline float square_dist(float x1, float y1, float x2, float y2) {
	return ((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

inline float randf() {
	return (rand()%10000) / 10000.f;
}

void rand_vector_array(Vector2* v, uint size, float min, float max) {
	for(uint i = 0; i < size; i++) {
		v[i].x = (randf() * (max - min)) + min;
		v[i].y = (randf() * (max - min)) + min;
	}
}

// randomize vector array with angle
void rand_dir_array(Vector2* v, uint size) {
	for(uint i = 0; i < size; i++) {
		float angle = randf() * 2 * PI;
		v[i].x = cos(angle);
		v[i].y = sin(angle);
	}
}

inline Vector2 Vector_norm(Vector2 v) { 
	float h = square_root((v.x * v.x) + (v.y * v.y)); 
	v.x = v.x/h; 
	v.y = v.y/h; 
	return v; 
}

int min(int x, int y) {
	return (x < y) * x + (x > y) * y;
}

int max(int x, int y) {
	return (x < y) * y + (x > y) * x;
}


//----------------------------------------------------------------------------------------------------------------------------------


// Player functions

void player_move(Camera2D* camera, float delta) {
	Vector2 mouse_pos;
	mouse_pos.x = GetMouseX();
	mouse_pos.y = GetMouseY();

	if(IsMouseButtonDown(0)) {
		camera->target.x -= (mouse_pos.x - mouse_pos_prev.x) / camera->zoom;
		camera->target.y -= (mouse_pos.y - mouse_pos_prev.y) / camera->zoom;
	}

	camera->target.x = Clamp(camera->target.x, -50, g_world_width+50);
	camera->target.y = Clamp(camera->target.y, -50, g_world_height+50);

	camera->offset.x = GetScreenWidth() / 2;
	camera->offset.y = GetScreenHeight()  / 2;

	int zoom_delta = (((-3 * GetMouseWheelMove()) | -IsKeyDown(KEY_Z)) | IsKeyDown(KEY_X));
	camera->zoom *= 1 - zoom_delta * 4.9715f * delta;
	camera->zoom = Clamp(camera->zoom, .15f, 1.f);

	mouse_pos_prev = mouse_pos;
}


//----------------------------------------------------------------------------------------------------------------------------------


// Population functions

Population* Population_create(ushort agent_count) {
	Population* population = (Population*) malloc(sizeof(Population));
	population->count = agent_count;

	population->positions = (Vector2*) malloc(sizeof(Vector2) * agent_count);
	population->directions = (Vector2*) malloc(sizeof(Vector2) * agent_count);
	population->infected_periods = (byte*) malloc(sizeof(byte) * agent_count);
	population->time_till_death = (byte*) malloc(sizeof(byte) * agent_count);
	population->simulated = (bool*) malloc(sizeof(bool) * agent_count);


	uint n = (uint) (agent_count-1);
	n = n*n*2;
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
		for(ushort j = 0; j < agent_count; j++) {
			square_distances[i*agent_count + j] = square_dist(positions[i].x, positions[i].y, positions[j].x, positions[j].y);
		}
	}
}

void agents_steer(Vector2* directions, Vector2* positions, bool* simulated, float* square_distances, ushort agent_count) {
	for(ushort i = 0; i < agent_count; i++) {
		Vector2 repulsion;
		repulsion.x = 0;
		repulsion.y = 0;

		// Add the vector opposite the direction of dots nearby prioritizing closer dots within the social distancing range
		for(ushort j = 0; j < agent_count; j++) {
			float dist = square_distances[i*agent_count + j];

			if(j == i || dist > g_social_distance * g_social_distance || !simulated[j])
				continue;

			repulsion.x += (positions[i].x - positions[j].x) / dist;
			repulsion.y += (positions[i].y - positions[j].y) / dist;
		}

		directions[i].x += repulsion.x * g_social_distance_factor;
		directions[i].y += repulsion.y * g_social_distance_factor;

		directions[i].x += ((randf() * 2) - 1.f) / 100.f;
		directions[i].y += ((randf() * 2) - 1.f) / 100.f;

		// Clamp the directions as to not result in infinite acceleration
		directions[i].x = Clamp(directions[i].x, -1, 1);
		directions[i].y = Clamp(directions[i].y, -1, 1);
	}

	// Bounce off walls
	for(ushort i = 0; i < agent_count; i++) {
			
		if((positions[i].x < 10 && directions[i].x < 0) || (positions[i].x > g_world_width - 10 && directions[i].x > 0)) {
			directions[i].x *= -1;
		}

		if((positions[i].y < 10 && directions[i].y < 0) || (positions[i].y > g_world_height - 10 && directions[i].y > 0)) {
			directions[i].y *= -1;
		}
	}
}

void agents_move(Vector2* directions, Vector2* positions, ushort agent_count, float delta) {
	for(ushort i = 0; i < agent_count; i++) {
		positions[i].x += directions[i].x * delta * 90.f;
		positions[i].y += directions[i].y * delta * 90.f; 
	}
}

// Add an "age" to determine how long the agent has been infected, this function runs once every tenth of a second and every tenth of a second has a 10% chance of incrementing the age by one. Meaning on average, the dots are incrementing their age by 1 every second. This is handled this way to distribute the agent's aging as to not result in huge spikes of mass death
void agents_age(byte* infected_periods, bool* simulated, byte* time_till_death, uint agent_count) {
	for(ushort i = 0; i < agent_count; i++) {
		if(infected_periods[i] > 0 && infected_periods[i] < g_infection_duration) {
			infected_periods[i] += (rand()%10==1);
		}
	}
	
	for(ushort i = 0; i < agent_count; i++) {
		simulated[i] *= (infected_periods[i] < time_till_death[i]);
	}
}

void agents_spread_disease(Vector2* positions, float* square_distances, byte* infected_periods, bool* simulated, byte* time_till_death, ushort agent_count) {
	// Agents must wait once second before able to spread disease as to prevent agents from infecting others the frame they become infected
	for(ushort i = 0; i < agent_count; i++) {
		if(infected_periods[i] == 1) {
			infected_periods[i]++;
		}
	}

	for(ushort i = 0; i < agent_count; i++) {
		if(infected_periods[i] > 0 && simulated[i]) {
			for(ushort j = 0; j < agent_count; j++) {
				if(infected_periods[j] == 0 && simulated[i]) {
					float dist = square_distances[i*agent_count + j];

					infected_periods[j] = randf() <= g_infection_chance && dist < (g_infection_radius * g_infection_radius);
					time_till_death[j] = (byte) g_infection_duration;
				}
			}
		}
	}
}

void agents_draw(Vector2* positions, byte* infected_periods, bool* simulated, ushort agent_count) {
	Color white_color = {  100 * g_social_distance_factor, 100 * g_social_distance_factor, 100 * g_social_distance_factor, 255};

	Color red_color = { 0 };
	red_color.r = 50 * g_infection_chance + 50;
	red_color.a = 255;

	for(ushort i = 0; i < agent_count; i++) {
		if(infected_periods[i] == 0) {
			DrawCircle(positions[i].x, positions[i].y, g_social_distance, white_color);
		}
	}

	for(ushort i = 0; i < agent_count; i++) {
		if(infected_periods[i] > 0 && simulated[i]) {
			DrawCircle(positions[i].x, positions[i].y, g_infection_radius, red_color);
		}
	}

	for(ushort i = 0; i < agent_count; i++) {
		if(!simulated[i]) {
			DrawCircle(positions[i].x, positions[i].y, 7, GRAY);
		}

		else if(infected_periods[i] > 0) {
			DrawCircle(positions[i].x, positions[i].y, 7, RED);
		}

		else {
			DrawCircle(positions[i].x, positions[i].y, 7, WHITE);
		}
	}
}

ushort agents_get_active_cases(byte* infected_periods, bool* simulated, uint agent_count) {
	ushort res = 0;

	for(ushort i = 0; i < agent_count; i++) {
		res += (infected_periods[i] > 0 && simulated[i]);
	}
	
	return res;
}

ushort agents_get_cases(byte* infected_periods, uint agent_count) {
	ushort res = 0;

	for(ushort i = 0; i < agent_count; i++) {
		res += (infected_periods[i] > 0);
	}
	
	return res;
}

ushort agents_get_removed(bool* simulated, uint agent_count) {
	ushort res = 0;

	for(ushort i = 0; i < agent_count; i++) {
		res += (!simulated[i]);
	}
	
	return res;
}

void agents_reset(Population* population) {
	for(uint i = 0; i < population->count; i++)
		population->infected_periods[i] = 0;

	for(uint i = 0; i < population->count; i++)
		population->simulated[i] = 1;

	rand_vector_array(population->positions, population->count, 0, g_world_width);
	rand_dir_array(population->directions, population->count);
}


//----------------------------------------------------------------------------------------------------------------------------------


int main() {
	SetConfigFlags(FLAG_MSAA_4X_HINT);	
	InitWindow(1280, 720, "Pandemic");

	Population* population = Population_create(1000);

	// Get pointers to all population arrays to simlify code later on
	Vector2* positions = population->positions;
	Vector2* directions = population->directions;
	byte* infected_periods = population->infected_periods;
	byte* time_till_death = population->time_till_death;
	float* square_distances = population->square_distances;
	bool* simulated = population->simulated;
	ushort agent_count = population->count;

	Font default_font;
	default_font = LoadFontEx("Bwana.otf", 30, 0, 0);
	SetTextureFilter(default_font.texture, FILTER_TRILINEAR);

	g_world_width = 3000;
	g_world_height = 3000;

	Camera2D camera = { 0 };
	camera.zoom = .231f;
	camera.target.x = (g_world_width / 2) - 550.f;
	camera.target.y = g_world_height / 2;

	float simulation_speed = 1.f;

	// Check for where the mouse is being used
	// 0 means nowhere
	// 1 means interacting with UI
	// 2 means in the world (Panning and zooming)
	byte cursor_focus = 0;

	Graph* total_cases_graph = Graph_create(400);
	Graph* active_cases_graph = Graph_create(400);
	Graph* removed_graph = Graph_create(400);

	Slider* simulation_speed_slider = Slider_create(15, 80, 300, 3, &simulation_speed, 0.f, 4.f);
	Slider* social_distance_slider = Slider_create(15, 80, 300, 3, &g_social_distance, 20.f, 120.f);
	Slider* social_distance_importance_slider = Slider_create(15, 10, 300, 3, &g_social_distance_factor, 0.f, 1.f);
	Slider* infection_radius_slider = Slider_create(15, 10, 300, 3, &g_infection_radius, 40.f, 120.f);
	Slider* infection_chance_slider = Slider_create(15, 10, 300, 3, &g_infection_chance, 0.05f, 1.f);
	Slider* infection_duration_slider = Slider_create(15, 10, 300, 3, &g_infection_duration, 5.f, 30.f);

	float counter = 0;
	float days = 1;
	float graph_counter = 0;
	float delta = 0;
	float prev_time = GetTime();

	uint total_cases;
	uint active_cases;
	uint removed;

	agents_reset(population);
	// Randomly infect one member of the population
	infected_periods[0] = 1;
	time_till_death[0] = (byte) g_infection_duration;

	while(!WindowShouldClose()) {
		float ui_ratio = GetScreenWidth() / 1280.f;
		ui_ratio = Clamp(ui_ratio, .85f, 1.f);

		delta = (GetTime() - prev_time);
		prev_time = GetTime();

		counter += delta * simulation_speed;
		graph_counter += delta * simulation_speed;

		if(delta > .2f) {
			printf("Game frozen\n");
			continue;
		}

		// Update UI
		if(IsMouseButtonReleased(0))
			cursor_focus = 0;

		else if(IsMouseButtonPressed(0) && GetMouseX() > 330 * ui_ratio)	
			cursor_focus = 2;

		else if(IsMouseButtonPressed(0) && GetMouseX() <= 330 * ui_ratio)	
			cursor_focus = 1;

		// Update interactable objects
		if(cursor_focus <= 1) {
			Slider_update(simulation_speed_slider);
			Slider_update(social_distance_slider);
			Slider_update(social_distance_importance_slider);
			Slider_update(infection_chance_slider);
			Slider_update(infection_duration_slider);
			Slider_update(infection_radius_slider);
		}

		// Handle player input
		if((GetMouseX() > 330 * ui_ratio && cursor_focus == 0) || cursor_focus == 2) {
			player_move(&camera, delta);
		}

		// Move the agents every frame
		agents_find_distances(square_distances, positions, agent_count);
		agents_steer(directions, positions, simulated, square_distances, agent_count);
		agents_move(directions, positions, agent_count, delta * simulation_speed);

		// On game tick
		if(counter > .1f) {
			// Spread disease
			agents_spread_disease(positions, square_distances, infected_periods, simulated, time_till_death, agent_count);
			agents_age(infected_periods, simulated, time_till_death, agent_count);
			days += .1f;
			counter = 0;
		}

		// Get disease spread information
		total_cases = agents_get_cases(infected_periods, agent_count);
		active_cases = agents_get_active_cases(infected_periods, simulated, agent_count);
		removed = agents_get_removed(simulated, agent_count);
		
		if(graph_counter > .2f) {
			// Update graph values
			Graph_add_point(total_cases_graph, total_cases);
			Graph_add_point(active_cases_graph, active_cases);
			Graph_add_point(removed_graph, removed);
			graph_counter = 0;
		}

		// Scale slider widths
		float slider_width = 310 * ui_ratio;
		simulation_speed_slider->width = slider_width;
		social_distance_slider->width = slider_width;
		social_distance_importance_slider->width = slider_width;
		infection_chance_slider->width = slider_width;
		infection_duration_slider->width = slider_width;
		infection_radius_slider->width = slider_width;
		
		// Rendering

		BeginDrawing();
		ClearBackground(BLACK);

		// Draw scene
		BeginMode2D(camera);

		agents_draw(positions, infected_periods, simulated, agent_count);
		DrawRectangleLinesEx((Rectangle) { 0, 0, g_world_width, g_world_height }, 4, WHITE);
		EndMode2D();


		// Draw UI

		// Draw graph section background
		
		float graph_section_width = (int) (330.f * ui_ratio);
		float graph_section_height = (int) (325.f * ui_ratio);

		DrawRectangle(5, 5, graph_section_width, graph_section_height, ui_dark_grey);
		float graph_width = 330.f * ui_ratio;
		float graph_height = 200.f * ui_ratio;

		// Draw graph section

		DrawRectangle(5, 5, (int) (330.f * ui_ratio), (int) (210.f * ui_ratio), ui_light_grey);

		Graph_draw(total_cases_graph, 5, 10, graph_width, graph_height, agent_count, 0.f, 10, 2.f, RED);
		Graph_draw(active_cases_graph, 5, 10, graph_width, graph_height, agent_count, 0.f, 10, 2.f, PURPLE);
		Graph_draw(removed_graph, 5, 10, graph_width, graph_height, agent_count, 0.f, 10, 2.f, GRAY);


		// Draw disease spread information

		char text[30];
		sprintf(text, "Total Cases: %i (%.1f%)", total_cases, ((float)total_cases / (float)agent_count) * 100.f);
		DrawTextEx(default_font, text, (Vector2) { 15, 30 + graph_height }, (int)(20.f * ui_ratio), 0, RED);

		sprintf(text, "Active Cases: %i (%.1f%)", active_cases, ((float)active_cases / (float)agent_count) * 100.f);
		DrawTextEx(default_font, text, (Vector2) { 15, 55 + graph_height }, (int)(20.f * ui_ratio), 0, PURPLE);

		sprintf(text, "Disease / Recovered: %i (%.1f%)", removed, ((float)removed / (float)agent_count) * 100.f);
		DrawTextEx(default_font, text, (Vector2) { 15, 80 + graph_height }, (int)(20.f * ui_ratio), 0, GRAY);

		sprintf(text, "Day: %i", (int) days);
		DrawTextEx(default_font, text, (Vector2) { 15, 105 + graph_height }, (int)(20.f * ui_ratio), 0, WHITE);


		// Draw slider section
		DrawRectangle(5, 315 + (30.f * ui_ratio), (int) (330.f * ui_ratio), (int) (295.f * ui_ratio), ui_dark_grey);
		
		// Draw each individual slider and it's text

		sprintf(text, "Simulation Speed (%.2f days/sec)", simulation_speed);
		DrawTextEx(default_font, text, (Vector2) { 15, 355 * ui_ratio }, 20 * ui_ratio, 0, WHITE);
		simulation_speed_slider->y = (380.f * ui_ratio);
		Slider_draw(simulation_speed_slider, WHITE, ui_light_grey);

		sprintf(text, "Social Distance (%.2fm)", (g_social_distance / 120) * 1.5f);
		DrawTextEx(default_font, text, (Vector2) { 15, 400 * ui_ratio }, 20 * ui_ratio, 0, WHITE);
		social_distance_slider->y = (430.f * ui_ratio);
		Slider_draw(social_distance_slider, WHITE, ui_light_grey);

		sprintf(text, "Social Distance Multipliyer (x%.2f)", g_social_distance_factor);
		DrawTextEx(default_font, text, (Vector2) { 15, 450 * ui_ratio }, 20 * ui_ratio, 0, WHITE);
		social_distance_importance_slider->y = (480.f * ui_ratio);
		Slider_draw(social_distance_importance_slider, WHITE, ui_light_grey);

		sprintf(text, "Infection Chance (%.1f%)", g_infection_chance * 100);
		DrawTextEx(default_font, text, (Vector2) { 15, 500 * ui_ratio }, 20 * ui_ratio, 0, RED);
		infection_chance_slider->y = (530.f * ui_ratio);
		Slider_draw(infection_chance_slider, RED, ui_light_grey);

		sprintf(text, "Infection Radius (%.2fm)", (g_infection_radius / 120) * 1.5f);
		DrawTextEx(default_font, text, (Vector2) { 15, 550 * ui_ratio }, 20 * ui_ratio, 0, RED);
		infection_radius_slider->y = (580.f * ui_ratio);
		Slider_draw(infection_radius_slider, RED, ui_light_grey);

		sprintf(text, "Infection Duration (~%.0f days)", (g_infection_duration));
		DrawTextEx(default_font, text, (Vector2) { 15, 600 * ui_ratio }, 20 * ui_ratio, 0, RED);
		infection_duration_slider->y = (630.f * ui_ratio);
		Slider_draw(infection_duration_slider, RED, ui_light_grey);

		DrawFPS(0, 0);
		EndDrawing();
	}

	// Free all memory used
	UnloadFont(default_font);

	Graph_destroy(total_cases_graph);
	Graph_destroy(active_cases_graph);
	Graph_destroy(removed_graph);

	Slider_destroy(simulation_speed_slider);
	Slider_destroy(social_distance_slider);
	Slider_destroy(social_distance_importance_slider);
	Slider_destroy(infection_chance_slider);
	Slider_destroy(infection_duration_slider);

	Population_destroy(population);

	// Why not?
	printf("Hello World!\n");
	return 0;
}
