#include <cmath>
#include <iostream>
#include "SDL3/SDL_render.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <numbers>
#include <ostream>
#include <utility>
#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

// in m/s^2
static float gravity_acceleration = 9.81;

static float frame_time = 1.0f / 10;

constexpr int width_height = 800;

constexpr float string_x = width_height / 2.0f;
constexpr float string_y = width_height / 5.0f;

constexpr int pixels_per_meter = 30;

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

static Vector v1 {3, 0};
static Vector v2 {4, std::numbers::pi / 2};

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

enum class VectorEndPointType {
    Point,
    Arrow,
    None
};

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

std::pair<int, int> draw_vector(Vector v, int sx, int sy, SDL_Renderer* renderer, VectorEndPointType end_type) {
    int dx = sx + v.horizontal_component() * pixels_per_meter;
    int dy = sy - v.vertical_component() * pixels_per_meter;

    SDL_RenderLine(renderer, sx, sy, dx, dy);

    switch (end_type) {
        case VectorEndPointType::Point:
            draw_point(renderer, {0, 255, 0, 0}, {dx, dy});
            break;
        case VectorEndPointType::Arrow: {
            float magnitude = -0.2;
            if (v.magnitude < 0)
                magnitude = 0.2;
            Vector v1 {magnitude, static_cast<float>(v.direction - std::numbers::pi / 4)};
            Vector v2 {magnitude, static_cast<float>(v.direction + std::numbers::pi / 4)};

            draw_vector(v1, dx, dy, renderer, VectorEndPointType::None);
            draw_vector(v2, dx, dy, renderer, VectorEndPointType::None);
            break;
        }
        case VectorEndPointType::None:
            break;
    }

    return {dx, dy};
}

void vector_edit(const char* title, Vector& v) {
    ImGui::Begin(title);

    ImGui::DragFloat("Magnitude (m)", &v.magnitude);
    ImGui::DragFloat("Angle (radians)", &v.direction);

    ImGui::End();
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
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    }

    ImGui_ImplSDL3_ProcessEvent(event);

    if (ImGui::GetIO().WantCaptureMouse) return SDL_APP_CONTINUE;
    if (ImGui::GetIO().WantCaptureKeyboard) return SDL_APP_CONTINUE;

    return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplSDLRenderer3_NewFrame();

    ImGui::NewFrame();

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    auto [dx, dy] = draw_vector(v1, width_height / 2, width_height / 2, renderer, VectorEndPointType::Point);
    draw_vector(v2, dx, dy, renderer, VectorEndPointType::Point);

    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    draw_vector(v1 + v2, width_height / 2, width_height / 2, renderer, VectorEndPointType::Arrow);

    vector_edit("Edit Vector 1", v1);
    vector_edit("Edit Vector 2", v2);

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
}
