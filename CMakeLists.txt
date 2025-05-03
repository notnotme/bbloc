cmake_minimum_required(VERSION 3.28)

project(bbloc VERSION 0.0.0 DESCRIPTION "Simple gpu rendering text editor.")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
# ========================================
add_executable(bbloc
        src/core/command/cvar/CVar.cpp
        src/core/command/cvar/CVarBool.cpp
        src/core/command/cvar/CVarColor.cpp
        src/core/command/cvar/CVarInt.cpp
        src/core/command/cvar/CVarFloat.cpp
        src/core/command/CommandManager.cpp
        src/core/command/ItemCallback.h
        src/core/cursor/buffer/StringBuffer.cpp
        src/core/cursor/buffer/VectorBuffer.cpp
        src/core/cursor/Cursor.cpp
        src/core/cursor/PromptCursor.cpp
        src/core/highlight/HighLighter.cpp
        src/core/renderer/AtlasArray.cpp
        src/core/renderer/QuadBuffer.cpp
        src/core/renderer/QuadProgram.cpp
        src/core/renderer/QuadTexture.cpp
        src/core/renderer/Shader.cpp
        src/core/ViewState.cpp
        src/core/theme/Theme.cpp
        src/editor/Editor.cpp
        src/editor/EditorState.cpp
        src/infobar/InfoBar.cpp
        src/prompt/Prompt.cpp
        src/prompt/PromptState.cpp
        src/ApplicationWindow.cpp
        src/main.cpp
)


# Dependencies
# ========================================
find_package(PkgConfig REQUIRED)

# SDL2
find_package(SDL2 REQUIRED)
target_link_libraries(bbloc PUBLIC SDL2::SDL2main SDL2::SDL2)

# OpenGL
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
target_link_libraries(bbloc PUBLIC OpenGL::GL)

# Freetype2
if (NOT NINTENDO_SWITCH)
    find_package(Freetype REQUIRED)
    target_link_libraries(bbloc PUBLIC Freetype::Freetype)
else ()
    pkg_check_modules(FREETYPE2 REQUIRED freetype2)
    target_include_directories(bbloc PUBLIC ${FREETYPE2_INCLUDE_DIRS})
    target_link_libraries(bbloc PUBLIC ${FREETYPE2_LIBRARIES})
endif ()

# tree-sitter
find_package(unofficial-tree-sitter CONFIG REQUIRED)
target_link_libraries(bbloc PUBLIC unofficial::tree-sitter::tree-sitter)
# tree-sitter languages
target_link_libraries(bbloc PUBLIC tree-sitter-cpp.a)
target_link_libraries(bbloc PUBLIC tree-sitter-json.a)

# utfcpp
find_package(utf8cpp REQUIRED)
target_link_libraries(bbloc PUBLIC utf8::cpp utf8cpp::utf8cpp)

# glad
if (NOT NINTENDO_SWITCH)
    find_package(glad CONFIG REQUIRED)
    target_link_libraries(bbloc PUBLIC glad::glad)
else ()
    pkg_check_modules(GLAD REQUIRED libglad)
    target_include_directories(bbloc PUBLIC ${GLAD_INCLUDE_DIRS})
    target_link_libraries(bbloc PUBLIC ${GLAD_LIBRARIES})
endif ()

if(NINTENDO_SWITCH)
    nx_generate_nacp(bbloc.nacp
        NAME ${CMAKE_PROJECT_NAME}
        VERSION ${CMAKE_PROJECT_VERSION}
        AUTHOR "romain337"
    )
    nx_create_nro(bbloc
        NACP bbloc.nacp
        ROMFS romfs
        ICON icon.jpg
    )
endif()

# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()