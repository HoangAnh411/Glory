#include "glory/engine/application.hpp"

#include "glory/engine/fixed_timestep.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <algorithm>
#include <cstdint>
#include <iostream>

namespace glory::engine {
namespace {

bool setOpenGLAttribute(SDL_GLAttr attribute, int value) {
    if (SDL_GL_SetAttribute(attribute, value)) {
        return true;
    }

    std::cerr << "SDL_GL_SetAttribute failed: " << SDL_GetError() << '\n';
    return false;
}

} // namespace

int runApplication(const ApplicationConfig& config, const FixedUpdateCallback& fixedUpdate) {
    if (config.windowWidth <= 0 || config.windowHeight <= 0 || !fixedUpdate) {
        std::cerr << "Invalid application configuration\n";
        return 1;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    if (!setOpenGLAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4) ||
        !setOpenGLAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5) ||
        !setOpenGLAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE) ||
        !setOpenGLAttribute(SDL_GL_DOUBLEBUFFER, 1) ||
        !setOpenGLAttribute(SDL_GL_DEPTH_SIZE, 24)) {
        SDL_Quit();
        return 1;
    }

    SDL_WindowFlags windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    if (config.hiddenWindow) {
        windowFlags |= SDL_WINDOW_HIDDEN;
    }
    SDL_Window* window = SDL_CreateWindow(
        "Glory - Lam Vuc",
        config.windowWidth,
        config.windowHeight,
        windowFlags);
    if (window == nullptr) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (context == nullptr) {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!SDL_GL_MakeCurrent(window, context)) {
        std::cerr << "SDL_GL_MakeCurrent failed: " << SDL_GetError() << '\n';
        SDL_GL_DestroyContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    int actualMajorVersion = 0;
    int actualMinorVersion = 0;
    int actualProfile = 0;
    const bool versionRead =
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &actualMajorVersion) &&
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &actualMinorVersion) &&
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &actualProfile);
    const bool versionSupported = actualMajorVersion > 4 ||
        (actualMajorVersion == 4 && actualMinorVersion >= 5);
    if (!versionRead || !versionSupported || actualProfile != SDL_GL_CONTEXT_PROFILE_CORE) {
        std::cerr << "OpenGL 4.5 Core context required; received "
                  << actualMajorVersion << '.' << actualMinorVersion
                  << " profile " << actualProfile << '\n';
        SDL_GL_DestroyContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!config.hiddenWindow && !SDL_ShowWindow(window)) {
        std::cerr << "SDL_ShowWindow failed: " << SDL_GetError() << '\n';
        SDL_GL_DestroyContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    const int swapInterval = config.smokeTestTickLimit == 0 ? 1 : 0;
    if (!SDL_GL_SetSwapInterval(swapInterval)) {
        std::cerr << "SDL_GL_SetSwapInterval warning: " << SDL_GetError() << '\n';
    }

    const std::uint64_t frequency = SDL_GetPerformanceFrequency();
    if (frequency == 0) {
        std::cerr << "SDL_GetPerformanceFrequency returned zero\n";
        SDL_GL_DestroyContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    FixedTimestep timestep;
    std::uint64_t completedTicks = 0;
    std::uint64_t previousCounter = SDL_GetPerformanceCounter();
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT ||
                event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED ||
                (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
                running = false;
            }
        }

        const std::uint64_t currentCounter = SDL_GetPerformanceCounter();
        const double frameSeconds = static_cast<double>(currentCounter - previousCounter) /
            static_cast<double>(frequency);
        previousCounter = currentCounter;

        timestep.advance(frameSeconds, [&](double fixedDeltaSeconds) {
            if (config.smokeTestTickLimit != 0 && completedTicks >= config.smokeTestTickLimit) {
                return;
            }
            fixedUpdate(fixedDeltaSeconds);
            ++completedTicks;
        });

        if (config.smokeTestTickLimit != 0 && completedTicks >= config.smokeTestTickLimit) {
            running = false;
        }

        int drawableWidth = 0;
        int drawableHeight = 0;
        SDL_GetWindowSizeInPixels(window, &drawableWidth, &drawableHeight);
        glViewport(0, 0, drawableWidth, drawableHeight);
        glClearColor(0.035F, 0.055F, 0.075F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

} // namespace glory::engine
