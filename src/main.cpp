#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <iostream>

SDL_Texture* SDL3_IMAGE_TEST(SDL_Renderer* renderer, int rOffset, int gOffset, int bVal, float scale);

// Global variables for ImGui manipulation
int   g_RedOffset   = 0;
int   g_GreenOffset = 0;
int   g_BlueValue   = 128;
float g_ScaleFactor = 1.0f;

int main(int argc, char* argv[])
{
     bool running = true;

    if (!SDL_Init(SDL_INIT_VIDEO)) 
    {
      SDL_Log("Error: SDL_Init(): %", SDL_GetError());
      running = false;
    }

    SDL_Window* window = SDL_CreateWindow("SDL3 Template", 1280, 720, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) 
    {
       SDL_Log("Error: SDL_CreateWindow(): %s", SDL_GetError());
       running = false;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) 
    {
       SDL_Log("Error: SDL_CreateRenderer(): %s", SDL_GetError());
       running = false;
    }

    SDL_SetRenderVSync(renderer, 1);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    // Setup scaling for high DPI
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;


    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) 
        {
             ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
            {
               running = false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) 
            {
               if (event.key.key == SDLK_ESCAPE && event.key.windowID == SDL_GetWindowID(window))
               {
                  running = false;
               }
            }
        }

         // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

          // --- GUI Content ---
        ImGui::Begin("Hello Window");

        // Button Logic: Returns true only on the click frame
        if (ImGui::Button("Say Hello")) {
           SDL_Log("Hello! Button was clicked.");
        }
        ImGui::End();

        // --- ImGui Controls ---
        ImGui::Begin("Texture Controls");
        ImGui::SliderInt("Red Offset", &g_RedOffset, -100, 100);
        ImGui::SliderInt("Green Offset", &g_GreenOffset, -100, 100);
        ImGui::SliderInt("Blue Value", &g_BlueValue, 0, 255);
        ImGui::SliderFloat("Scale Factor", &g_ScaleFactor, 0.1f, 2.0f);

        ImGui::End();
        // ----------------------

        // Cleanup previous frame's texture (important to avoid leaks when regenerating every frame)
        static SDL_Texture* currentTexture = nullptr;
        if (currentTexture) {
           SDL_DestroyTexture(currentTexture);
        }

        // Generate new texture with current ImGui values
        currentTexture = SDL3_IMAGE_TEST(renderer, g_RedOffset, g_GreenOffset, g_BlueValue, g_ScaleFactor);

        // ---- RENDERING ----
        // Example GUI
        ImGui::ShowDemoWindow();
        ImGui::Render();

        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColorFloat(renderer, 0.45f, 0.55f, 0.60f, 1.00f); // SDL3 uses float colors
        SDL_RenderClear(renderer);
        
        // Draw a red rectangle
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_FRect rect = { 200.0f, 150.0f, 200.0f, 150.0f };
        SDL_RenderFillRect(renderer, &rect);

        // Draw the procedurally generated texture scaled to fit window
        SDL_FRect destRect = { 0, 0, 1280, 720 };
        SDL_RenderTexture(renderer, currentTexture, NULL, &destRect);

        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        
        SDL_RenderPresent(renderer);
    }

     // Cleanup
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}   

SDL_Texture* SDL3_IMAGE_TEST(SDL_Renderer* renderer, int rOffset, int gOffset, int bVal, float scale)
{
   int width  = 1280;
   int height = 720;
   int pitch  = width * 4;

   // Allocate pixel data
   Uint32* pixels = (Uint32*)SDL_malloc(pitch * height);
   if (!pixels)
      return nullptr;

   // Generate pixel data using ImGui-controlled values
   for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
         int index = y * (pitch / 4) + x;

         // Apply ImGui controls to the formula
         Uint8 r = (Uint8)SDL_clamp((x * scale) + rOffset, 0, 255);
         Uint8 g = (Uint8)SDL_clamp((y * scale) + gOffset, 0, 255);
         Uint8 b = (Uint8)bVal;
         Uint8 a = 255;

         // Pack pixels (SDL_PIXELFORMAT_RGBA8888)
         pixels[index] = (a << 24) | (b << 16) | (g << 8) | r;
      }
   }

   // Create Surface
   SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA8888, pixels, pitch);
   if (!surface) 
   {
      SDL_free(pixels);
      return nullptr;
   }

   // Create/Update Texture
   // Hint: SDL_TEXTUREACCESS_STREAMING allows updates, but recreating is simpler for this demo
   SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);

   SDL_DestroySurface(surface);
   SDL_free(pixels); // CPU memory freed; GPU holds the copy

   return tex;
}