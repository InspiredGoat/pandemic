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
	float* infected_periods;
	bool* simulate;

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
float g_infection_radius = 22;
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

	camera->offset.x = GetScreenWidth() / 2;
	camera->offset.y = GetScreenHeight()  / 2;

	int zoom_delta = (((-3 * GetMouseWheelMove()) | -IsKeyDown(KEY_Z)) | IsKeyDown(KEY_X));
	camera->zoom *= 1 - zoom_delta * 4.9715f * delta;

	mouse_pos_prev = mouse_pos;
}


//----------------------------------------------------------------------------------------------------------------------------------


// Population functions

Population* Population_create(ushort agent_count) {
	Population* population = (Population*) malloc(sizeof(Population));
	population->count = agent_count;

	population->positions = (Vector2*) malloc(sizeof(Vector2) * agent_count);
	population->directions = (Vector2*) malloc(sizeof(Vector2) * agent_count);
	population->infected_periods = (byte*) malloc(sizeof(float) * agent_count);

	for(uint i = 0; i < agent_count; i++)
		population->infected_periods[i] = 0;

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

void agents_steer(Vector2* directions, Vector2* positions, byte* infected_periods, float* square_distances, ushort agent_count) {
	for(ushort i = 0; i < agent_count; i++) {
		Vector2 repulsion;
		repulsion.x = 0;
		repulsion.y = 0;

		// Add the vector opposite the direction of dots nearby prioritizing closer dots within the social distancing range
		for(ushort j = 0; j < agent_count; j++) {
			float dist = square_distances[i*agent_count + j];

			if(j == i || dist > g_social_distance * g_social_distance || infected_periods[j] >= g_infection_duration)
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
		for(ushort j = 0; j < g_section_count; j++) {
			
			if((positions[i].x < g_sections[j].x + 10 && directions[i].x < 0) || (positions[i].x > g_sections[j].width - 10 && directions[i].x > 0)) {
				directions[i].x *= -1;
			}

			if((positions[i].y < g_sections[j].y + 10 && directions[i].y < 0) || (positions[i].y > g_sections[j].height - 10 && directions[i].y > 0)) {
				directions[i].y *= -1;
			}
		}
	}
}

void agents_move(Vector2* directions, Vector2* positions, ushort agent_count, float delta) {
	for(ushort i = 0; i < agent_count; i++) {
		positions[i].x += directions[i].x * delta * 70.f;
		positions[i].y += directions[i].y * delta * 70.f; 
	}
}

// Add an "age" to determine how long the agent has been infected, this function runs once every tenth of a second and every tenth of a second has a 10% chance of incrementing the age by one. Meaning on average, the dots are incrementing their age by 1 every second. This is handled this way to distribute the agent's aging as to not result in huge spikes of mass death
void agents_age(byte* infected_periods, uint agent_count) {
	for(ushort i = 0; i < agent_count; i++) {
		if(infected_periods[i] > 0 && infected_periods[i] < g_infection_duration) {
			infected_periods[i] += (rand()%100<10);
		}
	}
}

void agents_spread_disease(Vector2* positions, float* square_distances, byte* infected_periods, ushort agent_count) {
	// Agents must wait once second before able to spread disease as to prevent agents from infecting others the frame they become infected
	for(ushort i = 0; i < agent_count; i++) {
		if(infected_periods[i] == 1) {
			infected_periods[i]++;
		}
	}

	for(ushort i = 0; i < agent_count; i++) {
		if(infected_periods[i] > 0 && infected_periods[i] < g_infection_duration) {
			for(ushort j = 0; j < agent_count; j++) {
				if(infected_periods[j] == 0) {
					float dist = square_distances[i*agent_count + j];

					if(randf() <= g_infection_chance && dist < (g_infection_radius * g_infection_radius))
						infected_periods[j] = 1;
				}
			}
		}
	}
}

void agents_draw(Vector2* positions, byte* infected_periods, ushort agent_count) {
	Color white_color = {  100 * g_social_distance_factor, 100 * g_social_distance_factor, 100 * g_social_distance_factor};
	white_color.a = 255;

	Color red_color = { 0 };
	red_color.r = 50 * g_infection_chance;
	red_color.a = 255;

	for(ushort i = 0; i < agent_count; i++) {
		if(infected_periods[i] == 0) {
			DrawCircle(positions[i].x, positions[i].y, g_social_distance, white_color);
		}
	}

	for(ushort i = 0; i < agent_count; i++) {
		if(infected_periods[i] > 0 && infected_periods[i] < g_infection_duration) {
			DrawCircle(positions[i].x, positions[i].y, g_infection_radius, red_color);
		}
	}

	for(ushort i = 0; i < agent_count; i++) {
		if(infected_periods[i] == g_infection_duration) {
			DrawCircle(positions[i].x, positions[i].y, 5, GRAY);
		}

		else if(infected_periods[i] > 0) {
			DrawCircle(positions[i].x, positions[i].y, 5, RED);
		}

		else {
			DrawCircle(positions[i].x, positions[i].y, 5, WHITE);
		}
	}
}

ushort agents_get_active_cases(byte* infected_periods, uint agent_count) {
	ushort res = 0;

	for(ushort i = 0; i < agent_count; i++) {
		res += (infected_periods[i] > 0 && infected_periods[i] < g_infection_duration);
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

ushort agents_get_removed(byte* infected_periods, uint agent_count) {
	ushort res = 0;

	for(ushort i = 0; i < agent_count; i++) {
		res += (infected_periods[i] >= g_infection_duration);
	}
	
	return res;
}

// Colors



//----------------------------------------------------------------------------------------------------------------------------------


int main() {
	SetConfigFlags(FLAG_MSAA_4X_HINT);	
	InitWindow(1280, 720, "Pandemic");

	Population* population = Population_create(1000);

	Vector2* positions = population->positions;
	Vector2* directions = population->directions;
	byte* infected_periods = population->infected_periods;
	float* square_distances = population->square_distances;
	ushort agent_count = population->count;

	Font default_font;
	default_font = LoadFontEx("Bwana.otf", 50, 0, 0);
	SetTextureFilter(default_font.texture, FILTER_TRILINEAR);

	Camera2D camera = { 0 };
	camera.zoom = .231f;
	camera.target.x = 863.64f;
	camera.target.y = 1496.37f;

	float val = 0;

	g_sections[0].x = 0;
	g_sections[0].y = 0;
	g_sections[0].width = 3000;
	g_sections[0].height = 3000;

	g_section_count = 1;

	// Check for where the mouse is being used
	// 0 means nowhere
	// 1 means interacting with UI
	// 2 means in the world (Panning and zooming)
	byte cursor_focus = 0;

	Graph* total_cases_graph = Graph_create(400);
	Graph* active_cases_graph = Graph_create(400);
	Graph* removed_graph = Graph_create(400);

	Slider* social_distance_slider = Slider_create(15, 80, 300, 3, &g_social_distance, 10.f, 100.f);
	Slider* social_distance_importance_slider = Slider_create(15, 10, 300, 3, &g_social_distance_factor, 0.f, 1.f);
	Slider* infection_radius_slider = Slider_create(15, 10, 300, 3, &g_infection_radius, 20.f, 50.f);
	Slider* infection_chance_slider = Slider_create(15, 10, 300, 3, &g_infection_chance, 0.05f, 1.f);
	Slider* infection_duration_slider = Slider_create(15, 10, 300, 3, &g_infection_duration, 5.f, 30.f);

	rand_vector_array(positions, agent_count, 0, g_sections[0].width);
	rand_dir_array(directions, agent_count);

	float counter = 0;
	float graph_counter = 0;
	float delta = 0;

	uint total_cases;
	uint active_cases;
	uint removed;

	// Randomly infect one member of the population
	infected_periods[0] = 1;

	while(!WindowShouldClose()) {
		float ui_ratio = GetScreenWidth() / 1280.f;
		ui_ratio = Clamp(ui_ratio, .75f, 1.f);

		delta = GetFrameTime();

		counter += delta;
		graph_counter += delta;


		// Update UI
		if(IsMouseButtonReleased(0))
			cursor_focus = 0;

		else if(IsMouseButtonPressed(0) && GetMouseX() > 300 * ui_ratio)	
			cursor_focus = 2;

		else if(IsMouseButtonPressed(0) && GetMouseX() <= 300 * ui_ratio)	
			cursor_focus = 1;

		// Update interactable objects
		if(cursor_focus <= 1) {
			Slider_update(social_distance_slider);
			Slider_update(social_distance_importance_slider);
			Slider_update(infection_chance_slider);
			Slider_update(infection_duration_slider);
			Slider_update(infection_radius_slider);
		}

		// Handle player input
		if((GetMouseX() > 300 * ui_ratio && cursor_focus == 0) || cursor_focus == 2) {
			player_move(&camera, delta);
		}

		// Move the agents every frame
		agents_find_distances(square_distances, positions, agent_count);
		agents_steer(directions, positions, infected_periods, square_distances, agent_count);
		agents_move(directions, positions, agent_count, delta);

		// On game tick
		if(counter > .1f) {
			// Spread disease
			agents_spread_disease(positions, square_distances, infected_periods, agent_count);
			agents_age(infected_periods, agent_count);
			counter = 0;
		}

		// Get disease spread information
		total_cases = agents_get_cases(infected_periods, agent_count);
		active_cases = agents_get_active_cases(infected_periods, agent_count);
		removed = agents_get_removed(infected_periods, agent_count);
		
		if(graph_counter > .2f) {
			// Update graph values
			Graph_add_point(total_cases_graph, total_cases);
			Graph_add_point(active_cases_graph, active_cases);
			Graph_add_point(removed_graph, removed);
			graph_counter = 0;
		}

		// Scale slider widths
		social_distance_slider->width = 300*ui_ratio;
		social_distance_importance_slider->width = 300*ui_ratio;
		infection_chance_slider->width = 300*ui_ratio;
		infection_duration_slider->width = 300*ui_ratio;
		infection_radius_slider->width = 300*ui_ratio;
		
		// Rendering

		BeginDrawing();
		ClearBackground(BLACK);

		// Draw scene
		BeginMode2D(camera);

		agents_draw(positions, infected_periods, agent_count);
		for(ushort i = 0; i < g_section_count; i++) {
			DrawRectangleLinesEx(g_sections[i], 4, WHITE);
		}
		EndMode2D();


		// Draw UI

		// Draw graph section background
		
		float graph_section_width = (int) (320.f * ui_ratio);
		float graph_section_height = (int) (320.f * ui_ratio);

		DrawRectangle(5, 5, graph_section_width, graph_section_height, ui_dark_grey);
		float graph_width = 315.f * ui_ratio;
		float graph_height = 200.f * ui_ratio;

		// Draw graph section

		DrawRectangle(5, 5, (int) (320.f * ui_ratio), (int) (210.f * ui_ratio), ui_light_grey);

		Graph_draw(total_cases_graph, 5, 10, graph_width, graph_height, agent_count, 0.f, 10, 2.f, RED);
		Graph_draw(active_cases_graph, 5, 10, graph_width, graph_height, agent_count, 0.f, 10, 2.f, PURPLE);
		Graph_draw(removed_graph, 5, 10, graph_width, graph_height, agent_count, 0.f, 10, 2.f, GRAY);


		// Draw disease spread information

		char text[20];
		sprintf(text, "Total Cases: %i", total_cases);
		DrawTextEx(default_font, text, (Vector2) { 15, 35 + graph_height }, (int)(25.f * ui_ratio), 0, RED);

		sprintf(text, "Cases: %i", active_cases);
		DrawTextEx(default_font, text, (Vector2) { 15, 65 + graph_height }, (int)(25.f * ui_ratio), 0, PURPLE);

		sprintf(text, "Diseased / Recovered: %i", removed);
		DrawTextEx(default_font, text, (Vector2) { 15, 95 + graph_height }, (int)(25.f * ui_ratio), 0, GRAY);


		// Draw slider section
		DrawRectangle(5, graph_section_height + (30.f * ui_ratio), (int) (320.f * ui_ratio), (int) (250.f * ui_ratio), ui_dark_grey);
		
		// Draw each individual slider and it's text

		DrawTextEx(default_font, "Social distance", (Vector2) { 15, 355 * ui_ratio }, 20, 0, WHITE);
		social_distance_slider->y = (380.f * ui_ratio);
		Slider_draw(social_distance_slider, WHITE, ui_light_grey);

		DrawTextEx(default_font, "Social distance multipliyer", (Vector2) { 15, 400 * ui_ratio }, 20, 0, WHITE);
		social_distance_importance_slider->y = (430.f * ui_ratio);
		Slider_draw(social_distance_importance_slider, WHITE, ui_light_grey);

		DrawTextEx(default_font, "Infectivity", (Vector2) { 15, 450 * ui_ratio }, 20, 0, RED);
		infection_chance_slider->y = (480.f * ui_ratio);
		Slider_draw(infection_chance_slider, RED, ui_light_grey);

		DrawTextEx(default_font, "Infection radius", (Vector2) { 15, 500 * ui_ratio }, 20, 0, RED);
		infection_radius_slider->y = (530.f * ui_ratio);
		Slider_draw(infection_radius_slider, RED, ui_light_grey);

		DrawTextEx(default_font, "Infection duration", (Vector2) { 15, 550 * ui_ratio }, 20, 0, RED);
		infection_duration_slider->y = (580.f * ui_ratio);
		Slider_draw(infection_duration_slider, RED, ui_light_grey);

		DrawFPS(0, 0);
		EndDrawing();
	}

	// Free all memory used
	UnloadFont(default_font);

	Graph_destroy(total_cases_graph);
	Graph_destroy(active_cases_graph);
	Graph_destroy(removed_graph);

	Slider_destroy(social_distance_slider);
	Slider_destroy(social_distance_importance_slider);
	Slider_destroy(infection_chance_slider);
	Slider_destroy(infection_duration_slider);
	Slider_destroy(infection_chance_slider);

	Population_destroy(population);

	// Why not?
	printf("Hello World!\n");
	return 0;
}
