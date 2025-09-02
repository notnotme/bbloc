cmake_minimum_required(VERSION 3.28)

project(bbloc VERSION 0.0.0 DESCRIPTION "Simple gpu rendering text editor.")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
# ========================================
add_executable(bbloc
        src/core/base/Command.h
        src/core/cursor/buffer/StringBuffer.cpp
        src/core/cursor/buffer/VectorBuffer.cpp
        src/core/cursor/Cursor.cpp
        src/core/cursor/PromptCursor.cpp
        src/core/cvar/CVarBool.cpp
        src/core/cvar/CVarColor.cpp
        src/core/cvar/CVarInt.cpp
        src/core/cvar/CVarFloat.cpp
        src/core/highlighter/HighLighter.cpp
        src/core/renderer/AtlasArray.cpp
        src/core/renderer/QuadBuffer.cpp
        src/core/renderer/QuadProgram.cpp
        src/core/renderer/QuadTexture.cpp
        src/core/renderer/Shader.cpp
        src/core/CommandManager.cpp
        src/core/CVarCommand.cpp
        src/core/ViewState.cpp
        src/core/theme/Theme.cpp
        src/command/QuitCommand.cpp
        src/command/OpenFileCommand.cpp
        src/command/SaveFileCommand.cpp
        src/command/ResetCVarFloatCommand.cpp
        src/command/FontSizeCommand.cpp
        src/command/SetHighLightCommand.cpp
        src/command/BindCommand.cpp
        src/command/ActivatePromptCommand.cpp
        src/command/CopyTextCommand.cpp
        src/command/PasteTextCommand.cpp
        src/command/CutTextCommand.cpp
        src/command/MoveCursorCommand.cpp
        src/command/ExecCommand.cpp
        src/command/CancelCommand.cpp
        src/command/ValidateCommand.cpp
        src/command/AutoCompleteCommand.cpp
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
target_link_libraries(bbloc PRIVATE SDL2::SDL2main SDL2::SDL2)

# OpenGL
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
target_link_libraries(bbloc PRIVATE OpenGL::GL)

# utfcpp
find_package(utf8cpp REQUIRED)
target_link_libraries(bbloc PRIVATE utf8::cpp utf8cpp::utf8cpp)

# tree-sitter
pkg_check_modules(TREE_SITTER REQUIRED tree-sitter)
target_include_directories(bbloc PRIVATE ${TREE_SITTER_INCLUDE_DIRS})
target_link_libraries(bbloc PRIVATE ${TREE_SITTER_LIBRARIES})

# tree-sitter cpp languages
pkg_check_modules(TREE_SITTER_CPP REQUIRED tree-sitter-cpp)
target_include_directories(bbloc PRIVATE ${TREE_SITTER_CPP_INCLUDE_DIRS})
target_link_libraries(bbloc PRIVATE ${TREE_SITTER_CPP_LIBRARIES})

# tree-sitter json languages
pkg_check_modules(TREE_SITTER_JSON REQUIRED tree-sitter-json)
target_include_directories(bbloc PRIVATE ${TREE_SITTER_JSON_INCLUDE_DIRS})
target_link_libraries(bbloc PRIVATE ${TREE_SITTER_JSON_LIBRARIES})

# Freetype2
pkg_check_modules(FREETYPE2 REQUIRED freetype2)
target_include_directories(bbloc PRIVATE ${FREETYPE2_INCLUDE_DIRS})
target_link_libraries(bbloc PRIVATE ${FREETYPE2_LIBRARIES})

# glad
pkg_check_modules(GLAD REQUIRED libglad)
target_include_directories(bbloc PRIVATE ${GLAD_INCLUDE_DIRS})
target_link_libraries(bbloc PRIVATE ${GLAD_LIBRARIES})

# NRO config
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

# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()