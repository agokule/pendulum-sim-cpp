#include "SDL3/SDL_render.h"
#include <cmath>
#include <iostream>
#include "imgui.h"
#include "imgui_stdlib.h"
#include <numbers>
#include <ostream>
#include <utility>
#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

// in meters
static float length = 10;
// in radians
static float theta_start = std::numbers::pi * 0.25;
// in radians
static float theta = theta_start;
// in m/s^2
static float gravity_acceleration = 9.81;

static float frame_time = 1.0f / 10;

constexpr int width_height = 800;

constexpr float string_x = width_height / 2.0f;
constexpr float string_y = width_height / 5.0f;

static int pixels_per_meter = 30;

struct Vector {
    float magnitude;
    // polar coordinates, in radians
    float direction;

    float horizontal_component() const {
        return magnitude * cos(direction);
    }
    float vertical_component() const {
        return magnitude * sin(direction);
    }
};

Vector operator*(Vector v, float s) {
    return { v.magnitude * s, v.direction };
}

Vector operator+(Vector v1, Vector v2) {
    if (v1.magnitude == 0)
        return v2;
    if (v2.magnitude == 0)
        return v1;

    float horizontal = v1.horizontal_component() + v2.horizontal_component();
    float vertical = v1.vertical_component() + v2.vertical_component();

    float magnitude = sqrt(horizontal * horizontal + vertical * vertical);
    float direction = atan2(vertical, horizontal);
    if (direction < 0)
        direction += 2 * std::numbers::pi;
    return { magnitude, direction };
}

std::ostream& operator<<(std::ostream& os, const Vector vector) {
    os << vector.magnitude << " m [" << vector.direction / std::numbers::pi << "󰏿]";
    return os;
}

static Vector acceleration = {0, 0};
static Vector velocity = {0, 0};

void draw_point(SDL_Renderer *renderer, SDL_Color color, std::pair<float, float> coordinates) {
    SDL_FRect frect {
        coordinates.first - 3.0f,
        coordinates.second - 3.0f,
        7.0, 7.0
    };
    bool success = SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    if (!success) std::cerr << "Error in SDL_SetRenderDrawColor: " << SDL_GetError();
    success = SDL_RenderFillRect(renderer, &frect);
    if (!success) std::cerr << "Error in SDL_RenderFillRect: " << SDL_GetError();
}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    /* Create the window */
    if (!SDL_CreateWindowAndRenderer("Pendulum Simulator", width_height, width_height, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderVSync(renderer, 2);

    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_KEY_DOWN ||
        event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    }
    return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    float sx = length * sin(theta);
    float sy = length * cos(theta);

    float x = sx * pixels_per_meter + string_x;
    float y = sy * pixels_per_meter + string_y;
    SDL_RenderLine(renderer, string_x, string_y, x, y);

    draw_point(renderer, {255, 0, 0, 255}, {x, y});

    Vector test1 {3, std::numbers::pi * 0};
    Vector test2 {4, std::numbers::pi * 1.5};
    Vector sum = test1 + test2;
    std::cout << sum << '\n';

    // 1 kg mass
    acceleration.magnitude = 1 * gravity_acceleration * sin(theta);
    acceleration.direction = theta + std::numbers::pi * 0.5f;
    std::cout << "acceleration: " << acceleration << '\n';

    Vector displacement = velocity * frame_time + acceleration * frame_time * frame_time * 0.5f;
    std::cout << "displacement: " << displacement << '\n';

    velocity = velocity + acceleration * frame_time;
    std::cout << "velocity: " << velocity << "\n";

    std::cout << "Before: " << "sx: " << sx << " m, sy: " << sy << " m\n";

    sx += displacement.horizontal_component();
    sy += displacement.vertical_component();

    std::cout << "After: " << "sx: " << sx << " m, sy: " << sy << " m\n";

    theta = atan2(sx, sy);
    std::cout << "theta: " << theta / std::numbers::pi << "󰏿 rad" << "\n\n";

    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
}
