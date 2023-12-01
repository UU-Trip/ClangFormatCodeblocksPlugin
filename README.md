# ClangFormat Code::Blocks plugin

This plugin integrates clang-format into your Code::Blocks projects.

## How it works

The plugin triggers whenever a file is saved, be it manually or by auto-save.
If a clang-format executable is present on the system, and the project contains a .clang-format file,
formatting is automatically applied to the saved file's contents according to the .clang-format file.
Formatting is applied "in place" by the editor. This means your undo-buffer is preserved.

## Build instructions for Linux

### Prerequisites

1. tinyxml2. Build and install it from source. -> https://github.com/leethomason/tinyxml2
2. Code::Blocks development libraries and headers. Install the codeblocks-devel package or build Code::Blocks from source.

### Building the plugin

1. Load up the .cbp project file in your favourite IDE. If your favourite IDE does not support .cbp files and is thus not Code::Blocks then get the hell out of my face.
2. In the project's global variables (Settings->Global variables):
    - Set the cb_include variable to Code::Block's include directory. By default, this will be something like /usr/include/codeblocks unless you've built Code::Blocks from source.

3. If you've built Code::Blocks from source:
    - Add the directory where libcodeblocks.so is located to the Linker search directories.

4. Hit the 'Build' button.

### Installing the plugin

1. Place ClangFormat.zip in the Code::Blocks resources dir. By default, this will be something like /usr/share/codeblocks unless you've built Code::Blocks from source.
2. Place libClangFormat.so in the Code::Blocks plugins dir. By default, this will be something like /usr/lib64/codeblocks/plugins unless you've built Code::Blocks from source.

## Using the plugin

The ClangFormat plugin is always active and will trigger whenever a C++ or C source file is saved.
The plugin may be disabled in the Plugin Manager under Plugins->Manage plugins.
