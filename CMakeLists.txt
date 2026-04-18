include(FetchContent)
cmake_minimum_required(VERSION 3.20)

project(PendulumSim LANGUAGES CXX)

# Fetch Version 3.4.4 of SDL
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG 5848e584a1b606de26e3dbd1c7e4ecbc34f807a6
    FIND_PACKAGE_ARGS # if SDL happens to be installed on the system, use that
)
FetchContent_MakeAvailable(SDL3)

set(SDL_TEST_LIBRARY OFF)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

add_executable(PendulumSim ${SOURCES})
if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")
endif()

target_compile_definitions(PendulumSim PRIVATE $<$<CONFIG:Debug>:DEBUG>)

target_include_directories(PendulumSim PRIVATE 
    src 
)

target_link_libraries(PendulumSim PRIVATE SDL3::SDL3)

# Handle copying SDL3 DLLs
if(WIN32)
    add_custom_command(
        TARGET PendulumSim POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:PendulumSim>
        VERBATIM
    )
endif()

