cmake_minimum_required(VERSION 3.10.0)
project(code VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Zoek Raylib
find_package(raylib REQUIRED)

# 2. Bronbestanden
file(GLOB SOURCES "src/*.cpp")

# 3. Het uitvoerbare bestand
add_executable(code ${SOURCES})

# 4. Include directories (voor ur_rtde, dxflib en je eigen headers)
target_include_directories(code PRIVATE include)

# 5. Libraries koppelen
# We voegen 'rtde' toe en de verplichte Windows netwerk-libs (ws2_32 en mswsock)
target_link_libraries(code PRIVATE 
    raylib 
    opengl32 
    gdi32 
    winmm
    ${CMAKE_SOURCE_DIR}/include/dxflib/libdxf.a
    ${CMAKE_SOURCE_DIR}/lib/librtde.dll.a   # Pad naar je nieuwe ur_rtde lib
    ws2_32                                     # Nodig voor ur_rtde netwerk
    mswsock                                    # Nodig voor ur_rtde netwerk
)
add_custom_command(TARGET code POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/lib/librtde.dll"   # Controleer of de naam exact ur_rtde.dll is
    $<TARGET_FILE_DIR:code>
)
