cmake_minimum_required(VERSION 3.25)
project(Glory LANGUAGES CXX)

include(FetchContent)
include(CTest)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.10
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3)

find_package(OpenGL REQUIRED)

add_library(glory_game
    game/src/simulation.cpp
)
target_include_directories(glory_game PUBLIC game/include)
target_compile_features(glory_game PUBLIC cxx_std_20)

add_library(glory_engine
    engine/src/application.cpp
)
target_include_directories(glory_engine PUBLIC engine/include)
target_compile_features(glory_engine PUBLIC cxx_std_20)
target_link_libraries(glory_engine PUBLIC SDL3::SDL3-static OpenGL::GL)

add_executable(glory_client game/src/main.cpp)
target_compile_features(glory_client PRIVATE cxx_std_20)
target_link_libraries(glory_client PRIVATE glory_engine glory_game)

if(BUILD_TESTING)
    add_executable(glory_fixed_timestep_tests tests/fixed_timestep_tests.cpp)
    target_include_directories(glory_fixed_timestep_tests PRIVATE engine/include)
    target_compile_features(glory_fixed_timestep_tests PRIVATE cxx_std_20)
    add_test(NAME glory_fixed_timestep COMMAND glory_fixed_timestep_tests)
endif()
