# bbloc

bbloc is a minimalist toy text editor developed in C++ using SDL2, OpenGL, glad, Freetype, utfcpp, and tree-sitter.

There are missing key feature but most importantly:
- copy/paste (must have for developper nowaday!)
- undo/redo
- search/replace
- More and better syntax highlight (mappers need to be done)
- Better error handling.

### Main components:

- **CommandManager**  
Commands can be registered to become available in the prompt. There is also a CVar system inspired from game engines. They are shared pointers living in random parts of the code and accessible from the CommandManager. Some variables and theme attributes can be changed at runtime using this system. The command prompt also supports a simple feedback from the user (depth: 1), and eventually displays one message after command execution. It also features auto-completion for commands and arguments and also provides some utilities like strings tokenization.

  Current commands implemented are:  
  |Command|Description|
  |-|-|
  |**open \<filename\>**|Open the said file in the editor. Try to set the highlight mode the corresponding language (via file extension).|
  |**save \<filename\> -f**|Save the current editor to disk with the said filename. Will ask for feedback if the file exists. If **-f** is used, will not ask for feedback and save directly (overwrite the existing file).|
  |**quit**|No arguments, exit the program without saving.|
  |**cvar \<name\> [value1] [value2] ...**|If only the cvar name is provided, prints its value in the prompt. Otherwise, will try to set the new value on the CVar.|
  |**reset_render_time**|No arguments. The CVar **inf_render_time** is read-only, and this command set its value back to 0. You can later check the value to see how much time was spent for rendering the window.|
  |**exec \<filename\>**|This will read the said file line by line and execute each command in it. There is some limitation: the exec will stop if feedback is needed, or if a message is returned from one the command in the list. Commands inside the exec file are not parts of history.|
  |**set_hl_mode \<mode\>**|Change highlight mode in the editor. Please do not try to enable color highlight with a gigantic regular text file as the application does not set any timer to stop long parsing operation yet.|

  Current available CVar are:
  |Variable|Type|Description|
  |-|-|-|
  |**inf_render_time**|1 float|**Read-only.** Holds the maximum render time in seconds since last reset.|
  |**tab_to_space**|1 boolean|If true "\t" character is replaced by **dim_tab_to_space** space character(s).|
  |**col_\***|4 byte: R G B A|Holds a theme color attribute.|
  |**dim_\***|1 int|Holds a theme dimension attribute.|

- **Cursor**  
The cursor holds a unique pointer to a TextBuffer implementation, which is responsible for storing the text data, insert and delete operation on it. In addition, the Cursor provides the caret location, and move functions, insert and delete functions (which use the TextBuffer implementation), and some utilities. After each operation on the Cursor that changes the underlying text, a BufferEdit struct is returned (backed by TextBuffer) so it can be passed to the highlighter to update its internal parse tree.

- **HighLighter**  
This uses tree-sitter for the text highlight feature. Nothing fancy, the most simplistic approach is used: tint character by their symbol representation. This is not ideal (like JSON showing same color for key:value, with strings) but seems to be working for now. The HighLighter is bound to a Cursor so it can read text from it. If the Cursor's text is edited, pass the BufferEdit object to the HighLighter so it can re-read the changed part and update the syntax color.

- **Renderer**  
The renderer use OpenGL and instantiated rendering. The glyph atlas is generated on the fly using the freetype library and populate a texture array used by the OpenGL renderer. One generated vertex in the application results in a textured and tinted quad on screen. It sticks to OpenGL 4.3 to be able to keep the Nintendo Switch support.

- **Theme**  
This holds convenient function to access theme attributes like background or highlight colors and dimensions. It allows to change the font size, measure text width in pixels, and provide characters information (texture coordinates, size, ect).

- **Views**  
There is no real view system, but a base class that holds common data for the rendering parts of the application (info bar, editor, and prompt). They are tied to a ViewState which holds the data that the View needs for its purpose. SDL input events are passed to Views to they can manage user inputs.

- **ApplicationWindow**  
Manage the application resources, SDL main loop, and pump events to update the views accordingly.

### Dependencies

This use some external libray:

- [**SDL2**](https://github.com/libsdl-org/SDL) is used for the input and windowing system.
- [**Glad**](https://glad.dav1d.de/) is used as dynamic function loader for OpenGL.
- [**Freetype**](https://github.com/freetype) is used to generate font glyphs.
- [**utfcpp**](https://github.com/nemtrif/utfcpp) is used to convert text back and forth from UTF-8 to UTF-16.
- [**tree-sitter**](https://github.com/tree-sitter/tree-sitter) is used for syntax highlighting.
    - [**tree-sitter-cpp**](https://github.com/tree-sitter/tree-sitter-cpp) language for c/cpp syntax highlight.
    - [**tree-sitter-json**](https://github.com/tree-sitter/tree-sitter-json) language for JSON syntax highlight.
- This repository ships with [**JetBrains Mono**](https://www.jetbrains.com/lp/mono/) ttf font.

### How to build
For linux, using CLion and the VCPKG integration should be straightforward (import project, set vcpkg, import necessary libraries, compile and debug.). Otherwise, you can use cmake as usual.

The Nintendo Switch version needs devkitpro with devkitA64 and utilities + libraries. You'll need to compile and install *utfcpp*, *tree-sitter* and the *tree-sitter parsers* yourself.

To run the program, change the path **"romfs/"** to **"romfs:/"** in the **ApplicationWindow.cpp** source file.

Assuming your using Linux:
```
$ mkdir nx && cd nx
$ source $DEVKITPRO/switchvars.sh
$ cmake -G"Unix Makefiles" -DCMAKE_C_FLAGS="$CFLAGS $CPPFLAGS" -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/Switch.cmake ..
$ make
```

Keep in mind that the game controller is not yet supported, so you must connect a keyboard via USB. I only test in portable mode **without** keyboard.