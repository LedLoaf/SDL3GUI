#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>

SDL_Texture* SDL3_IMAGE_TEST(SDL_Renderer* renderer);

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("SDL3 Template", 640, 480, 0);
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    

	auto tex = SDL3_IMAGE_TEST(renderer);


    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Draw a red rectangle
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_FRect rect = { 200.0f, 150.0f, 200.0f, 150.0f };
        SDL_RenderFillRect(renderer, &rect);

        // Draw the procedurally generated texture scaled to fit window
        SDL_FRect destRect = { 0, 0, 640, 480 };
        SDL_RenderTexture(renderer, tex, NULL, &destRect);


        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}   

SDL_Texture* SDL3_IMAGE_TEST(SDL_Renderer* renderer)
{
    // 1. Define dimensions and pitch (bytes per row)
    int width = 256;
    int height = 256;
    int pitch = width * 4; // 4 bytes per pixel (RGBA8888)

    // 2. Allocate pixel data in CPU memory
    Uint32* pixels = (Uint32*)SDL_malloc(pitch * height);
    if (!pixels) {
        printf("Failed to allocate pixel memory\n");
        return nullptr;
    }

    // 3. Generate pixel data (Example: Horizontal Gradient)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * (pitch / 4) + x;
            Uint8 r = (Uint8)x;       // Red increases horizontally
            Uint8 g = (Uint8)y;       // Green increases vertically
            Uint8 b = 128;            // Constant Blue
            Uint8 a = 255;            // Opaque

            // Pack into 32-bit integer (Little Endian: ABGR or RGBA depending on format)
            // SDL_PIXELFORMAT_RGBA8888 expects R in lowest byte on LE systems usually, 
            // but SDL_MapRGBA is safer for cross-platform compatibility.
            // Here we manually pack for SDL_PIXELFORMAT_RGBA8888:
            pixels[index] = (a << 24) | (b << 16) | (g << 8) | r;
        }
    }

    // 4. Create SDL_Surface from existing pixel data (No file loaded)
    // SDL_CreateSurfaceFrom wraps the data; it does not copy it.
    SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height,
        SDL_PIXELFORMAT_RGBA8888,
        pixels, pitch);
    if (!surface) {
        printf("SDL_CreateSurfaceFrom Error: %s\n", SDL_GetError());
        SDL_free(pixels);
        return nullptr;
    }

    // 5. Create GPU Texture from the Surface
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
    if (!tex) {
        printf("SDL_CreateTextureFromSurface Error: %s\n", SDL_GetError());
        SDL_DestroySurface(surface);
        SDL_free(pixels);
        return nullptr;
    }

    // Cleanup CPU surface and raw pixel data immediately after texture creation
    // The texture has its own copy of the pixel data on the GPU.
    SDL_DestroySurface(surface);
    SDL_free(pixels);

    return tex;
}