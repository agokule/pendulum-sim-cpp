#include <cmath>
#include <iostream>
#include <format>
#include "SDL3/SDL_render.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <limits>
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

// in kg
static float mass = 1;

constexpr int width_height = 900;

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

Vector operator*(Vector v, float s) {
    return { v.magnitude * s, v.direction };
}

Vector operator/(Vector v, float s) {
    return { v.magnitude / s, v.direction };
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
static Vector tension_force = {0, 0};
static Vector velocity = {0, 0};

static bool show_acceleration = true;
static bool show_tension = false;
static bool show_velocity = false;
static bool show_displacement = true;
static bool show_weight_force = false;
static bool show_vector_labels = true;
static bool paused = false;

struct PendulumString: public Vector {
    float starting_magnitude;
    float starting_direction;

    PendulumString(float magnitude)
        : Vector{magnitude, std::numbers::pi * 1.75}, starting_magnitude{magnitude}, starting_direction {direction}
    {}

    PendulumString operator+=(Vector v) {
        Vector v2 = Vector{magnitude, direction} + v;
        magnitude = v2.magnitude;
        direction = v2.direction;

        return *this;
    }
};
static PendulumString pendulum_string {10.0f};

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

std::pair<int, int> draw_vector(Vector v, int sx, int sy, SDL_Renderer* renderer, VectorEndPointType end_type, const char* label = nullptr) {
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

    if (label && show_vector_labels) {
        ImGui::PushFont(nullptr, 11.0f);
        ImGui::GetForegroundDrawList()->AddText(ImVec2(dx + 5, dy + 5), IM_COL32(255, 255, 255, 255), label);
        ImGui::PopFont();
    }

    return {dx, dy};
}

void reset() {
    pendulum_string.direction = pendulum_string.starting_direction;
    pendulum_string.magnitude = pendulum_string.starting_magnitude;
    velocity = {0, 0};
    acceleration = {0, 0};
    tension_force = {0, 0};
}

void pendulum_edit(const char* title, PendulumString& v) {
    ImGui::Begin(title);

    ImGui::Text("Current length: %.3f meters", v.magnitude);
    ImGui::Text("Current angle: %.3f radians", v.direction);

    ImGui::PushItemWidth(60.0f);

    bool changed = false;
    changed |= ImGui::DragFloat("Length (meters)", &v.starting_magnitude, 0.3, 0.4, 100);
    changed |= ImGui::DragFloat("Angle (radians)", &v.starting_direction, 0.1, std::numbers::pi * 1.01, 1.99 * std::numbers::pi);

    // mass actually doesn't change anything, so we are ignoring it for changed bool
    ImGui::DragFloat("Mass of Bob (kg)", &mass, 0.4, 0.001, std::numeric_limits<float>::infinity());

    ImGui::PopItemWidth();

    if (changed)
        reset();

    ImGui::Checkbox("Show vector labels", &show_vector_labels);

    ImGui::Checkbox("Show resultant acceleration vector", &show_acceleration);

    ImGui::Checkbox("Show tension force vector**", &show_tension);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("tension and weight are scaled 0.1x for better visibility");

    ImGui::Checkbox("Show weight force vector**", &show_weight_force);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("tension and weight are scaled 0.1x for better visibility");

    ImGui::Checkbox("Show velocity vector", &show_velocity);

    ImGui::Checkbox("Show displacement vector**", &show_displacement);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("displacement is scaled 100x for better visibility");

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
    // SDL_SetRenderVSync(renderer, 2);
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

void tick_once(float frame_time) {
    auto [dx, dy] = draw_vector(pendulum_string, string_x, string_y, renderer, VectorEndPointType::Point);
    acceleration.magnitude = gravity_acceleration;
    acceleration.direction = std::numbers::pi * 1.5;
    if (show_weight_force)
        draw_vector(acceleration * mass / 10, dx, dy, renderer, VectorEndPointType::Arrow, std::format("Weight Force ({:.2f} N)", (acceleration * mass).magnitude).c_str());

                        // mg cos(theta)
    tension_force.magnitude = mass * gravity_acceleration * cos(pendulum_string.direction - std::numbers::pi * 1.5)
                        // mv^2 / r
                        + mass * velocity.magnitude * velocity.magnitude / pendulum_string.starting_magnitude;
    tension_force.direction = pendulum_string.direction - std::numbers::pi;

    if (show_tension)
        draw_vector(tension_force / 10, dx, dy, renderer, VectorEndPointType::Arrow, std::format("Tension Force ({:.2f} N)", tension_force.magnitude).c_str());
    acceleration = acceleration + (tension_force / mass);
    if (show_acceleration)
        draw_vector(acceleration, dx, dy, renderer, VectorEndPointType::Arrow, std::format("Acceleration ({:.2f} m/s^2)", acceleration.magnitude).c_str());

    Vector displacement = velocity * frame_time + acceleration * frame_time * frame_time * 0.5;
    if (show_displacement)
        draw_vector(displacement * 100, dx, dy, renderer, VectorEndPointType::Arrow, std::format("Displacement ({:.1e} m)", displacement.magnitude).c_str());

    pendulum_string += displacement;

    velocity = velocity + acceleration * frame_time;
    if (show_velocity)
        draw_vector(velocity, dx, dy, renderer, VectorEndPointType::Arrow, std::format("Velocity ({:.2f} m/s)", velocity.magnitude).c_str());

    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);

}

static auto last_time = SDL_GetTicksNS();

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplSDLRenderer3_NewFrame();

    ImGui::NewFrame();

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    float time = (SDL_GetTicksNS() - last_time) / 10e9;
    last_time = SDL_GetTicksNS();
    if (paused)
        time = 0;
    tick_once(time);
    pendulum_edit("Edit The Pendulum", pendulum_string);

    // Bottom-centered controls
    ImGui::SetNextWindowPos(ImVec2(width_height / 2.0f, width_height - 20.0f), ImGuiCond_Always, ImVec2(0.5f, 1.0f));
    ImGui::SetNextWindowSize(ImVec2(0, 0)); // auto-size
    ImGui::Begin("##controls", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    if (ImGui::Button(paused ? "Play" : "Pause"))
        paused = !paused;
    ImGui::SameLine();
    if (ImGui::Button("Reset"))
        reset();
    ImGui::End();

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
}
