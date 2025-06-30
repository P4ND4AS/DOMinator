cmake_minimum_required(VERSION 3.14)
project(BookMap-mk1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Où placer les exécutables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Inclure tous les .cpp du dossier src et sous-dossiers
file(GLOB_RECURSE SOURCES
    src/*.cpp
    src/glad.c
)

# Inclure les headers
include_directories(
    include
    src
    include/glad
    include/GLFW
    include/glm
    include/imgui
    include/freetype
    include/text
    include/densities
    include/engine
    include/KHR
    include/UI
)

# Détection du compilateur pour choisir le dossier de libs
if(MSVC)
    set(LIB_SUBDIR "msvc")
elseif(MINGW)
    set(LIB_SUBDIR "mingw")
else()
    set(LIB_SUBDIR "")
endif()

if (WIN32)
    set(LIB_DIR "${PROJECT_SOURCE_DIR}/lib/${LIB_SUBDIR}")
    link_directories(${LIB_DIR})
endif()

# Dépendances système
find_package(OpenGL REQUIRED)

# Ajoute l'exécutable
add_executable(BookMap-mk1 ${SOURCES})

# Linking des librairies
if (WIN32)
    target_link_libraries(BookMap-mk1
        OpenGL::GL
        glfw3
        freetype
        gdi32
    )
else()
    target_link_libraries(BookMap-mk1
        OpenGL::GL
        glfw
        freetype
        pthread
        dl
        m
    )
endif()

# Copie les DLL dans le dossier de build sous Windows
if (WIN32)
    add_custom_command(TARGET BookMap-mk1 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${LIB_DIR}/glfw3.dll"
            "${LIB_DIR}/freetype.dll"
            $<TARGET_FILE_DIR:BookMap-mk1>
    )
endif()

# Ajoute le dossier assets et fonts au binaire (pour retrouver les images et polices)
add_custom_command(TARGET BookMap-mk1 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:BookMap-mk1>/assets"
)
add_custom_command(TARGET BookMap-mk1 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/fonts"
        "$<TARGET_FILE_DIR:BookMap-mk1>/fonts"
)

message(STATUS "Configure done. To build: 
    mkdir build && cd build
    cmake ..
    cmake --build . 
L'exécutable sera dans build/bin/")