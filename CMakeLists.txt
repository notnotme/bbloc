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

# utfcpp
find_package(utf8cpp REQUIRED)
target_link_libraries(bbloc PUBLIC utf8::cpp utf8cpp::utf8cpp)

# Freetype2
find_package(Freetype REQUIRED)
target_link_libraries(bbloc PUBLIC Freetype::Freetype)

# glad
find_package(glad CONFIG REQUIRED)
target_link_libraries(bbloc PUBLIC glad::glad)

# tree-sitter
pkg_check_modules(TREE_SITTER REQUIRED tree-sitter)
target_include_directories(bbloc PUBLIC ${TREE_SITTER_INCLUDE_DIRS})
target_link_libraries(bbloc PUBLIC ${TREE_SITTER_LIBRARIES})

# tree-sitter cpp languages
pkg_check_modules(TREE_SITTER_CPP REQUIRED tree-sitter-cpp)
target_include_directories(bbloc PUBLIC ${TREE_SITTER_CPP_INCLUDE_DIRS})
target_link_libraries(bbloc PUBLIC ${TREE_SITTER_CPP_LIBRARIES})

# tree-sitter json languages
pkg_check_modules(TREE_SITTER_JSON REQUIRED tree-sitter-json)
target_include_directories(bbloc PUBLIC ${TREE_SITTER_JSON_INCLUDE_DIRS})
target_link_libraries(bbloc PUBLIC ${TREE_SITTER_JSON_LIBRARIES})

# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()