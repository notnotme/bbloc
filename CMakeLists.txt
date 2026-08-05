cmake_minimum_required(VERSION 3.28)

project(bbloc VERSION 0.0.0 DESCRIPTION "Simple gpu rendering text editor.")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile commands for clang-tidy, reachable from the repository root
# (the symlink always tracks the most recently configured build).
# Module scanning is off: no modules used, and its GCC-only flags break clang-tidy.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
file(CREATE_LINK "${CMAKE_BINARY_DIR}/compile_commands.json" "${CMAKE_SOURCE_DIR}/compile_commands.json" SYMBOLIC)

# Source files
# ========================================
add_executable(bbloc
        src/core/base/Command.h
        src/core/cursor/buffer/LongestLineTracker.cpp
        src/core/cursor/buffer/LineBuffer.cpp
        src/core/cursor/Cursor.cpp
        src/core/cursor/PromptCursor.cpp
        src/core/cursor/SurrogatePair.h
        src/core/cursor/UndoHistory.cpp
        src/core/cvar/CVarBool.cpp
        src/core/cvar/CVarColor.cpp
        src/core/cvar/CVarInt.cpp
        src/core/cvar/CVarFloat.cpp
        src/core/highlighter/HighLighter.cpp
        src/core/highlighter/Parser.cpp
        src/core/highlighter/ParserCatalog.cpp
        src/core/renderer/AtlasArray.cpp
        src/core/renderer/QuadBuffer.cpp
        src/core/renderer/QuadProgram.cpp
        src/core/renderer/QuadTexture.cpp
        src/core/renderer/Shader.cpp
        src/core/CommandManager.cpp
        src/core/CursorContextManager.cpp
        src/core/CVarCommand.cpp
        src/core/ViewState.cpp
        src/core/theme/TabStop.h
        src/core/theme/Theme.cpp
        src/command/QuitCommand.cpp
        src/command/OpenFileCommand.cpp
        src/command/SaveFileCommand.cpp
        src/command/SearchCommand.cpp
        src/command/ResetCVarFloatCommand.cpp
        src/command/FontSizeCommand.cpp
        src/command/SetHighLightCommand.cpp
        src/command/BindCommand.cpp
        src/command/BufferCommand.cpp
        src/command/ActivatePromptCommand.cpp
        src/command/CopyTextCommand.cpp
        src/command/PasteTextCommand.cpp
        src/command/CutTextCommand.cpp
        src/command/UndoCommand.cpp
        src/command/RedoCommand.cpp
        src/command/MoveCursorCommand.cpp
        src/command/GotoLineCommand.cpp
        src/command/ExecCommand.cpp
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

# Freetype2
find_package(Freetype REQUIRED)
target_link_libraries(bbloc PRIVATE Freetype::Freetype)

# glad
find_package(glad CONFIG REQUIRED)
target_link_libraries(bbloc PRIVATE glad::glad)

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

# tree-sitter ini languages
pkg_check_modules(TREE_SITTER_INI REQUIRED tree-sitter-ini)
target_include_directories(bbloc PRIVATE ${TREE_SITTER_INI_INCLUDE_DIRS})
target_link_libraries(bbloc PRIVATE ${TREE_SITTER_INI_LIBRARIES})

# tree-sitter yaml languages
pkg_check_modules(TREE_SITTER_YAML REQUIRED tree-sitter-yaml)
target_include_directories(bbloc PRIVATE ${TREE_SITTER_YAML_INCLUDE_DIRS})
target_link_libraries(bbloc PRIVATE ${TREE_SITTER_YAML_LIBRARIES})

# tree-sitter toml languages
pkg_check_modules(TREE_SITTER_TOML REQUIRED tree-sitter-toml)
target_include_directories(bbloc PRIVATE ${TREE_SITTER_TOML_INCLUDE_DIRS})
target_link_libraries(bbloc PRIVATE ${TREE_SITTER_TOML_LIBRARIES})

# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()