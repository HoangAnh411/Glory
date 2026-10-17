cmake_minimum_required(VERSION 3.25)
project(Glory LANGUAGES C CXX)

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

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
    enet
    GIT_REPOSITORY https://github.com/lsalzman/enet.git
    GIT_TAG v1.3.18
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(enet)

FetchContent_Declare(
    sqlite3
    URL https://www.sqlite.org/2024/sqlite-amalgamation-3460100.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(sqlite3)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 2c980bb59875b0d32144a71867fbdebb2f77cd20
    GIT_SHALLOW FALSE
)
FetchContent_Declare(
    cgltf
    GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
    GIT_TAG v1.14
    GIT_SHALLOW TRUE
)
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.21
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(stb cgltf)
FetchContent_GetProperties(miniaudio)
if(NOT miniaudio_POPULATED)
    FetchContent_Populate(miniaudio)
endif()
find_package(OpenGL REQUIRED)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.8
    GIT_SHALLOW TRUE
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

add_library(glory_imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(glory_imgui PUBLIC ${imgui_SOURCE_DIR})
target_link_libraries(glory_imgui PUBLIC SDL3::SDL3-static OpenGL::GL)

add_library(glory_game
    game/src/action_map.cpp
    game/src/arena.cpp
    game/src/boss.cpp
    game/src/character_controller.cpp
    game/src/classes.cpp
    game/src/collision.cpp
    game/src/combat.cpp
    game/src/dungeon.cpp
    game/src/inventory.cpp
    game/src/prediction.cpp
    game/src/spatial_grid.cpp
    game/src/social.cpp
    game/src/simulation.cpp
    game/src/status_effect.cpp
    game/src/world_history.cpp
    game/src/world_encounter.cpp
)
target_include_directories(glory_game PUBLIC game/include shared/include)
target_compile_features(glory_game PUBLIC cxx_std_20)
target_link_libraries(glory_game PUBLIC glm::glm nlohmann_json::nlohmann_json)

add_library(glory_network network/src/network.cpp)
target_include_directories(glory_network PUBLIC network/include ${enet_SOURCE_DIR}/include)
target_compile_features(glory_network PUBLIC cxx_std_20)
target_link_libraries(glory_network PUBLIC enet)
if(WIN32)
    target_link_libraries(glory_network PUBLIC ws2_32 winmm)
endif()

add_library(glory_sqlite STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
target_include_directories(glory_sqlite PUBLIC ${sqlite3_SOURCE_DIR})
set_target_properties(glory_sqlite PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES)

add_library(glory_server_services
    server/src/database_backup.cpp
    server/src/structured_logger.cpp
)
target_include_directories(glory_server_services PUBLIC server/include)
target_compile_features(glory_server_services PUBLIC cxx_std_20)
target_link_libraries(glory_server_services PRIVATE glory_sqlite)

add_library(glory_db server/src/account_database.cpp)
target_include_directories(glory_db PUBLIC server/include)
target_compile_features(glory_db PUBLIC cxx_std_20)
target_link_libraries(glory_db PUBLIC glory_network PRIVATE glory_sqlite nlohmann_json::nlohmann_json)
if(WIN32)
    target_link_libraries(glory_db PRIVATE bcrypt)
else()
    find_package(OpenSSL REQUIRED COMPONENTS Crypto)
    target_link_libraries(glory_db PRIVATE OpenSSL::Crypto)
endif()

add_library(glory_engine
    engine/src/animation.cpp
    engine/src/application.cpp
    engine/src/audio_system.cpp
    engine/src/camera.cpp
    engine/src/cgltf_impl.cpp
    engine/src/debug_panel.cpp
    engine/src/directional_light.cpp
    engine/src/gl_api.cpp
    engine/src/gltf_asset.cpp
    engine/src/image_loader.cpp
    engine/src/input_system.cpp
    engine/src/mesh.cpp
    engine/src/model.cpp
    engine/src/model_renderer.cpp
    engine/src/render_pipeline.cpp
    engine/src/miniaudio_impl.cpp
    engine/src/particle_renderer.cpp
    engine/src/particle_system.cpp
    engine/src/pbr_math.cpp
    engine/src/shader_program.cpp
    engine/src/shadow_map.cpp
    engine/src/shadow_math.cpp
    engine/src/skybox.cpp
    engine/src/stb_image_impl.cpp
    engine/src/stb_image_write_impl.cpp
    engine/src/texture2d.cpp
    engine/src/ground_plane.cpp
)
target_include_directories(glory_engine
    PUBLIC engine/include shared/include
    PRIVATE ${stb_SOURCE_DIR} ${cgltf_SOURCE_DIR} ${miniaudio_SOURCE_DIR}
)
target_compile_features(glory_engine PUBLIC cxx_std_20)
target_link_libraries(glory_engine PUBLIC SDL3::SDL3-static OpenGL::GL glm::glm nlohmann_json::nlohmann_json PRIVATE glory_imgui)

add_executable(glory_client game/src/main.cpp)
target_compile_definitions(glory_client PRIVATE GLORY_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")
target_compile_features(glory_client PRIVATE cxx_std_20)
target_link_libraries(glory_client PRIVATE glory_engine glory_game glory_network)

add_executable(glory_server
    server/src/main.cpp
    server/src/server_anti_cheat.cpp
    server/src/server_simulation.cpp
)
target_include_directories(glory_server PRIVATE server/include)
target_compile_definitions(glory_server PRIVATE GLORY_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")
target_compile_features(glory_server PRIVATE cxx_std_20)
target_link_libraries(glory_server PRIVATE glory_game glory_network glory_db glory_server_services)

add_library(glory_load_test
    server/src/load_test.cpp
    server/src/server_anti_cheat.cpp
    server/src/server_simulation.cpp
)
target_include_directories(glory_load_test PUBLIC server/include)
target_compile_definitions(glory_load_test PRIVATE GLORY_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")
target_compile_features(glory_load_test PUBLIC cxx_std_20)
target_link_libraries(glory_load_test PUBLIC glory_game glory_network)

add_executable(glory_bot_runner tools/bot_runner.cpp)
target_link_libraries(glory_bot_runner PRIVATE glory_load_test)
target_compile_features(glory_bot_runner PRIVATE cxx_std_20)

if(WIN32)
    # Tự động copy runtime DLLs cạnh executable để chạy trực tiếp không bị thiếu DLL
    get_filename_component(COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    foreach(DLL_NAME libunwind.dll libc++.dll)
        if(EXISTS "${COMPILER_DIR}/${DLL_NAME}")
            configure_file("${COMPILER_DIR}/${DLL_NAME}" "${CMAKE_BINARY_DIR}/${DLL_NAME}" COPYONLY)
        endif()
    endforeach()
endif()

if(BUILD_TESTING)
    add_executable(glory_fixed_timestep_tests tests/fixed_timestep_tests.cpp)
    target_include_directories(glory_fixed_timestep_tests PRIVATE engine/include)
    target_compile_features(glory_fixed_timestep_tests PRIVATE cxx_std_20)
    add_test(NAME glory_fixed_timestep COMMAND glory_fixed_timestep_tests)

    add_executable(glory_input_mapping_tests tests/input_mapping_tests.cpp)
    target_link_libraries(glory_input_mapping_tests PRIVATE glory_game)
    target_compile_features(glory_input_mapping_tests PRIVATE cxx_std_20)
    add_test(NAME glory_input_mapping COMMAND glory_input_mapping_tests)

    add_executable(glory_camera_tests tests/camera_tests.cpp)
    target_link_libraries(glory_camera_tests PRIVATE glory_engine)
    target_compile_features(glory_camera_tests PRIVATE cxx_std_20)
    add_test(NAME glory_camera COMMAND glory_camera_tests)

    add_executable(glory_asset_loader_tests tests/asset_loader_tests.cpp)
    target_compile_definitions(glory_asset_loader_tests PRIVATE GLORY_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")
    target_link_libraries(glory_asset_loader_tests PRIVATE glory_engine)
    target_compile_features(glory_asset_loader_tests PRIVATE cxx_std_20)
    add_test(NAME glory_asset_loader COMMAND glory_asset_loader_tests)

    add_executable(glory_directional_light_tests tests/directional_light_tests.cpp)
    target_link_libraries(glory_directional_light_tests PRIVATE glory_engine)
    target_compile_features(glory_directional_light_tests PRIVATE cxx_std_20)
    add_test(NAME glory_directional_light COMMAND glory_directional_light_tests)

    add_executable(glory_skeletal_animation_tests tests/skeletal_animation_tests.cpp)
    target_link_libraries(glory_skeletal_animation_tests PRIVATE glory_engine)
    target_compile_features(glory_skeletal_animation_tests PRIVATE cxx_std_20)
    target_compile_definitions(glory_skeletal_animation_tests PRIVATE GLORY_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")
    add_test(NAME glory_skeletal_animation COMMAND glory_skeletal_animation_tests)

    add_executable(glory_shadow_tests tests/shadow_tests.cpp engine/src/shadow_math.cpp)
    target_include_directories(glory_shadow_tests PRIVATE engine/include)
    target_link_libraries(glory_shadow_tests PRIVATE glm::glm)
    target_compile_features(glory_shadow_tests PRIVATE cxx_std_20)
    add_test(NAME glory_shadow COMMAND glory_shadow_tests)

    add_executable(glory_pbr_tests tests/pbr_tests.cpp engine/src/pbr_math.cpp)
    target_include_directories(glory_pbr_tests PRIVATE engine/include)
    target_link_libraries(glory_pbr_tests PRIVATE glm::glm)
    target_compile_features(glory_pbr_tests PRIVATE cxx_std_20)
    add_test(NAME glory_pbr COMMAND glory_pbr_tests)

    add_executable(glory_particle_tests tests/particle_tests.cpp engine/src/particle_system.cpp)
    target_include_directories(glory_particle_tests PRIVATE engine/include)
    target_link_libraries(glory_particle_tests PRIVATE glm::glm)
    target_compile_features(glory_particle_tests PRIVATE cxx_std_20)
    add_test(NAME glory_particle COMMAND glory_particle_tests)

    add_executable(glory_character_controller_tests tests/character_controller_tests.cpp)
    target_link_libraries(glory_character_controller_tests PRIVATE glory_game)
    target_compile_features(glory_character_controller_tests PRIVATE cxx_std_20)
    add_test(NAME glory_character_controller COMMAND glory_character_controller_tests)

    add_executable(glory_third_person_camera_tests tests/third_person_camera_tests.cpp)
    target_include_directories(glory_third_person_camera_tests PRIVATE engine/include)
    target_link_libraries(glory_third_person_camera_tests PRIVATE glm::glm)
    target_compile_features(glory_third_person_camera_tests PRIVATE cxx_std_20)
    add_test(NAME glory_third_person_camera COMMAND glory_third_person_camera_tests)

    add_executable(glory_collision_tests tests/collision_tests.cpp)
    target_link_libraries(glory_collision_tests PRIVATE glory_game)
    target_compile_definitions(glory_collision_tests PRIVATE GLORY_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")
    target_compile_features(glory_collision_tests PRIVATE cxx_std_20)
    add_test(NAME glory_collision COMMAND glory_collision_tests)

    add_executable(glory_combat_tests tests/combat_tests.cpp)
    target_link_libraries(glory_combat_tests PRIVATE glory_game)
    target_compile_definitions(glory_combat_tests PRIVATE GLORY_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")
    target_compile_features(glory_combat_tests PRIVATE cxx_std_20)
    add_test(NAME glory_combat COMMAND glory_combat_tests)

    add_executable(glory_skill_registry_tests tests/skill_registry_tests.cpp)
    target_link_libraries(glory_skill_registry_tests PRIVATE glory_game)
    target_compile_definitions(glory_skill_registry_tests PRIVATE GLORY_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")
    target_compile_features(glory_skill_registry_tests PRIVATE cxx_std_20)
    add_test(NAME glory_skill_registry COMMAND glory_skill_registry_tests)

    add_executable(glory_world_encounter_tests tests/world_encounter_tests.cpp)
    target_link_libraries(glory_world_encounter_tests PRIVATE glory_game)
    target_compile_definitions(glory_world_encounter_tests PRIVATE GLORY_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")
    target_compile_features(glory_world_encounter_tests PRIVATE cxx_std_20)
    add_test(NAME glory_world_encounter COMMAND glory_world_encounter_tests)

    add_executable(glory_server_simulation_tests
        tests/server_simulation_tests.cpp
        server/src/server_anti_cheat.cpp
        server/src/server_simulation.cpp
    )
    target_include_directories(glory_server_simulation_tests PRIVATE server/include)
    target_compile_definitions(glory_server_simulation_tests PRIVATE GLORY_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")
    target_link_libraries(glory_server_simulation_tests PRIVATE glory_game)
    target_compile_features(glory_server_simulation_tests PRIVATE cxx_std_20)
    add_test(NAME glory_server_simulation COMMAND glory_server_simulation_tests)

    add_executable(glory_status_effect_tests tests/status_effect_tests.cpp)
    target_link_libraries(glory_status_effect_tests PRIVATE glory_game)
    target_compile_features(glory_status_effect_tests PRIVATE cxx_std_20)
    add_test(NAME glory_status_effect COMMAND glory_status_effect_tests)

    add_executable(glory_behavior_tree_tests tests/behavior_tree_tests.cpp)
    target_link_libraries(glory_behavior_tree_tests PRIVATE glm::glm)
    target_include_directories(glory_behavior_tree_tests PRIVATE game/include)
    target_compile_features(glory_behavior_tree_tests PRIVATE cxx_std_20)
    add_test(NAME glory_behavior_tree COMMAND glory_behavior_tree_tests)

    add_executable(glory_boss_tests tests/boss_tests.cpp)
    target_link_libraries(glory_boss_tests PRIVATE glory_game)
    target_compile_features(glory_boss_tests PRIVATE cxx_std_20)
    add_test(NAME glory_boss COMMAND glory_boss_tests)

    add_executable(glory_network_tests tests/network_tests.cpp)
    target_link_libraries(glory_network_tests PRIVATE glory_network)
    target_compile_features(glory_network_tests PRIVATE cxx_std_20)
    add_test(NAME glory_network COMMAND glory_network_tests)

    add_executable(glory_server_authoritative_tests tests/server_authoritative_tests.cpp
        server/src/server_anti_cheat.cpp
        server/src/server_simulation.cpp)
    target_include_directories(glory_server_authoritative_tests PRIVATE server/include)
    target_compile_definitions(glory_server_authoritative_tests PRIVATE GLORY_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")
    target_link_libraries(glory_server_authoritative_tests PRIVATE glory_network glory_game)
    target_compile_features(glory_server_authoritative_tests PRIVATE cxx_std_20)
    add_test(NAME glory_server_authoritative COMMAND glory_server_authoritative_tests)

    add_executable(glory_snapshot_interpolation_tests tests/snapshot_interpolation_tests.cpp)
    target_link_libraries(glory_snapshot_interpolation_tests PRIVATE glory_network)
    target_compile_features(glory_snapshot_interpolation_tests PRIVATE cxx_std_20)
    add_test(NAME glory_snapshot_interpolation COMMAND glory_snapshot_interpolation_tests)

    add_executable(glory_prediction_reconciliation_tests tests/prediction_reconciliation_tests.cpp)
    target_link_libraries(glory_prediction_reconciliation_tests PRIVATE glory_game)
    target_compile_features(glory_prediction_reconciliation_tests PRIVATE cxx_std_20)
    add_test(NAME glory_prediction_reconciliation COMMAND glory_prediction_reconciliation_tests)

    add_executable(glory_lag_compensation_tests tests/lag_compensation_tests.cpp)
    target_link_libraries(glory_lag_compensation_tests PRIVATE glory_game)
    target_compile_features(glory_lag_compensation_tests PRIVATE cxx_std_20)
    add_test(NAME glory_lag_compensation COMMAND glory_lag_compensation_tests)

    add_executable(glory_arena_matchmaking_tests tests/arena_matchmaking_tests.cpp
        server/src/server_anti_cheat.cpp
        server/src/server_simulation.cpp)
    target_include_directories(glory_arena_matchmaking_tests PRIVATE server/include)
    target_compile_definitions(glory_arena_matchmaking_tests PRIVATE GLORY_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")
    target_link_libraries(glory_arena_matchmaking_tests PRIVATE glory_network glory_game)
    target_compile_features(glory_arena_matchmaking_tests PRIVATE cxx_std_20)
    add_test(NAME glory_arena_matchmaking COMMAND glory_arena_matchmaking_tests)

    add_executable(glory_account_database_tests tests/account_database_tests.cpp)
    target_link_libraries(glory_account_database_tests PRIVATE glory_db glory_network)
    target_compile_features(glory_account_database_tests PRIVATE cxx_std_20)
    add_test(NAME glory_account_database COMMAND glory_account_database_tests)

    add_executable(glory_inventory_equipment_tests tests/inventory_equipment_tests.cpp)
    target_compile_definitions(glory_inventory_equipment_tests PRIVATE GLORY_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")
    target_link_libraries(glory_inventory_equipment_tests PRIVATE glory_game glory_db glory_network)
    target_compile_features(glory_inventory_equipment_tests PRIVATE cxx_std_20)
    add_test(NAME glory_inventory_equipment COMMAND glory_inventory_equipment_tests)

    add_executable(glory_zone_interest_tests tests/zone_interest_tests.cpp)
    target_link_libraries(glory_zone_interest_tests PRIVATE glory_game glory_network)
    target_compile_features(glory_zone_interest_tests PRIVATE cxx_std_20)
    add_test(NAME glory_zone_interest COMMAND glory_zone_interest_tests)

    add_executable(glory_social_system_tests tests/social_system_tests.cpp)
    target_link_libraries(glory_social_system_tests PRIVATE glory_game glory_db glory_network)
    target_compile_features(glory_social_system_tests PRIVATE cxx_std_20)
    add_test(NAME glory_social_system COMMAND glory_social_system_tests)

    add_executable(glory_dungeon_instance_tests tests/dungeon_instance_tests.cpp)
    target_link_libraries(glory_dungeon_instance_tests PRIVATE glory_game glory_network)
    target_compile_features(glory_dungeon_instance_tests PRIVATE cxx_std_20)
    add_test(NAME glory_dungeon_instance COMMAND glory_dungeon_instance_tests)

    add_executable(glory_marketplace_tests tests/marketplace_tests.cpp)
    target_link_libraries(glory_marketplace_tests PRIVATE glory_db glory_network glory_sqlite)
    target_compile_features(glory_marketplace_tests PRIVATE cxx_std_20)
    add_test(NAME glory_marketplace COMMAND glory_marketplace_tests)

    add_executable(glory_character_class_balance_tests tests/character_class_balance_tests.cpp)
    target_compile_definitions(glory_character_class_balance_tests PRIVATE GLORY_ASSET_DIR=\"${CMAKE_SOURCE_DIR}/assets\")
    target_link_libraries(glory_character_class_balance_tests PRIVATE glory_game)
    target_compile_features(glory_character_class_balance_tests PRIVATE cxx_std_20)
    add_test(NAME glory_character_class_balance COMMAND glory_character_class_balance_tests)

    add_executable(glory_anti_cheat_tests
        tests/anti_cheat_tests.cpp
        server/src/server_anti_cheat.cpp
        server/src/server_simulation.cpp
    )
    target_include_directories(glory_anti_cheat_tests PRIVATE server/include game/include)
    target_compile_definitions(glory_anti_cheat_tests PRIVATE GLORY_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")
    target_link_libraries(glory_anti_cheat_tests PRIVATE glory_game glory_network)
    target_compile_features(glory_anti_cheat_tests PRIVATE cxx_std_20)
    add_test(NAME glory_anti_cheat COMMAND glory_anti_cheat_tests)

    add_executable(glory_load_test_bot_tests tests/load_test_bot_tests.cpp)
    target_link_libraries(glory_load_test_bot_tests PRIVATE glory_load_test)
    target_compile_features(glory_load_test_bot_tests PRIVATE cxx_std_20)
    add_test(NAME glory_load_test_bots COMMAND glory_load_test_bot_tests)

    add_executable(glory_logging_backup_tests tests/logging_backup_tests.cpp)
    target_link_libraries(glory_logging_backup_tests PRIVATE glory_server_services glory_db glory_sqlite)
    target_compile_features(glory_logging_backup_tests PRIVATE cxx_std_20)
    add_test(NAME glory_logging_backup COMMAND glory_logging_backup_tests)

    add_executable(glory_closed_beta_feedback_tests tests/closed_beta_feedback_tests.cpp)
    target_link_libraries(glory_closed_beta_feedback_tests PRIVATE glory_db glory_network glory_sqlite nlohmann_json::nlohmann_json)
    target_compile_features(glory_closed_beta_feedback_tests PRIVATE cxx_std_20)
    add_test(NAME glory_closed_beta_feedback COMMAND glory_closed_beta_feedback_tests)

    add_executable(glory_game_hud_tests tests/test_game_hud.cpp)
    target_link_libraries(glory_game_hud_tests PRIVATE glory_engine glory_game)
    target_compile_features(glory_game_hud_tests PRIVATE cxx_std_20)
    add_test(NAME glory_game_hud COMMAND glory_game_hud_tests)
endif()
