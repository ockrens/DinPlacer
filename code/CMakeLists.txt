cmake_minimum_required(VERSION 3.10.0)
project(code VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Bronbestanden indexeren
file(GLOB SOURCES "src/*.cpp")

# 2. Het uitvoerbare bestand definiëren
add_executable(DinPlacer ${SOURCES})

# 3. Globale Include mappen (Lokale headers)
target_include_directories(DinPlacer PRIVATE ${CMAKE_SOURCE_DIR}/include)

# 4. Platform-specifieke configuratie
if(WIN32)   
    message(STATUS "Configuring voor Windows-build...")

    # Op Windows zoeken we Raylib via de standaard find_package methode
    find_package(raylib REQUIRED)

    # Linken met Windows libraries en de libraries uit de map lib/windows
    target_link_libraries(DinPlacer PRIVATE 
        raylib 
        opengl32 
        gdi32 
        winmm
        ws2_32                                     
        mswsock                                    
        ${CMAKE_SOURCE_DIR}/lib/windows/libdxf.a
        ${CMAKE_SOURCE_DIR}/lib/windows/librtde.dll.a
    )

    # Kopieer de Windows DLL automatisch naar de map waar code.exe wordt gebouwd
    add_custom_command(TARGET DinPlacer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/lib/windows/librtde.dll"
        $<TARGET_FILE_DIR:DinPlacer>
    )

elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Configuring voor Linux (Raspberry Pi OS / WSL) build...")

    # Op Linux gebruiken we pkg-config om Windows-paden volledig te omzeilen
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(RAYLIB REQUIRED raylib)

    # Threads is verplicht voor ur_rtde op Linux
    find_package(Threads REQUIRED)

    # Handmatig de include-paden van de Linux Raylib-installatie toevoegen
    target_include_directories(DinPlacer PRIVATE ${RAYLIB_INCLUDE_DIRS})

    # Linken met Linux libraries en de libraries uit de map lib/linux
    target_link_libraries(DinPlacer PRIVATE 
        ${RAYLIB_LIBRARIES}
        Threads::Threads
        ${CMAKE_SOURCE_DIR}/lib/linux/libdxf.a
        ${CMAKE_SOURCE_DIR}/lib/linux/librtde.so
        ${CMAKE_SOURCE_DIR}/lib/linux/liburcl.so
    )

    # RPATH instellen: Dit bakt het bibliotheekpad direct in het uitvoerbare bestand.
    # Hierdoor weet het programma bij het opstarten altijd waar librtde.so en liburcl.so staan.
    set_target_properties(DinPlacer PROPERTIES 
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "$ORIGIN/../lib/linux"
    )

else()
    # Harde foutmelding als het noch Windows, noch Linux is
    message(FATAL_ERROR "Fout: Dit platform wordt niet ondersteund. Gebruik Windows of Linux (Raspberry Pi OS)!")
endif()
