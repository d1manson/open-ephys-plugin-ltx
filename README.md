# Open Ephys Plugin - LTX Record Engine

The LTX format is broadly compatible with **L**egacy analysis code used with **T**etrode recordings in various rodent labs. However it is _not_ e**X**plicitly defined.
Amongst other pre-existing tools, the LTX files should be viewable within the [Waveform GUI](https://github.com/d1manson/waveform).

Roughly speaking, all files contain multiple header lines of the format: `key<space>value`, followed by the special token `data_start`, then binary data, and then `data_end`.

It produces:

- experiment_name.set - a file that only contains header info.
- experiment_name.1, experiment_name.2, ... - tetrode spike data. For each spike, the binary data gives `4 x [4 byte timestamp | 50 one-byte voltage values]`.
- experiment_name.efg, experiment_name.efg2, ... - continuous data downsampled to 1kHz and stored as single bytes without any timestamp.
- experiment_name.pos - position data. Expects 6 continuous data channels from bonsai containing x1,y1,x2,y2,numpix1,numpix2 respectively. Currently assumes
   the x and y values are within the range [0,1000]. To get the data from bonsai into Openephys, use the separate [bonsai plugin](https://github.com/d1manson/open-ephys-plugin-bonsai)
   (note there are a few open ephys plugins that aim to stich together Bonsai and Openephys, so make sure you are using the right one).

Note that you'll need three separate nodes in openephys, one to create set+tet file, one for eeg, and one for pos.

**WARNING**: this does pretty much work, but the timestamp logic still needs to be ironed out, and there's currently no way to configure anything here.


----

Template readme...


# Record Engine Plugin Template

This repository contains a template for building **Record Engine** plugins for the [Open Ephys GUI](https://github.com/open-ephys/plugin-GUI). Record Engine plugins allow the GUI's Record Node to write data into a new format. By default, the GUI ships with Record Engines for the Binary and Open Ephys formats.

Information on the Open Ephys Plugin API can be found on [the GUI's documentation site](https://open-ephys.github.io/gui-docs/Developer-Guide/Open-Ephys-Plugin-API.html).

## Creating a new Record Engine Plugin

1. Click "Use this template" to instantiate a new repository under your GitHub account. 
2. Clone the new repository into a directory at the same level as the `plugin-GUI` repository. This is typically named `OEPlugins`, but it can have any name you'd like.
3. Modify the [OpenEphysLib.cpp file](https://open-ephys.github.io/gui-docs/Developer-Guide/Creating-a-new-plugin.html) to include your plugin's name and version number.
4. Create the plugin [build files](https://open-ephys.github.io/gui-docs/Developer-Guide/Compiling-plugins.html) using CMake.
5. Use Visual Studio (Windows), Xcode (macOS), or `make` (Linux) to compile the plugin.
6. Edit the code to add custom functionality, and add additional source files as needed.

## Repository structure

This repository contains 3 top-level directories:

- `Build` - Plugin build files will be auto-generated here. These files will be ignored in all `git` commits.
- `Source` - All plugin source files (`.h` and `.cpp`) should live here. There can be as many source code sub-directories as needed.
- `Resources` - This is where you should store any non-source-code files, such as library files or scripts.

## Using external libraries

To link the plugin to external libraries, it is necessary to manually edit the Build/CMakeLists.txt file. The code for linking libraries is located in comments at the end.
For most common libraries, the `find_package` option is recommended. An example would be

```cmake
find_package(ZLIB)
target_link_libraries(${PLUGIN_NAME} ${ZLIB_LIBRARIES})
target_include_directories(${PLUGIN_NAME} PRIVATE ${ZLIB_INCLUDE_DIRS})
```

If there is no standard package finder for cmake, `find_library`and `find_path` can be used to find the library and include files respectively. The commands will search in a variety of standard locations For example

```cmake
find_library(ZMQ_LIBRARIES NAMES libzmq-v120-mt-4_0_4 zmq zmq-v120-mt-4_0_4) #the different names after names are not a list of libraries to include, but a list of possible names the library might have, useful for multiple architectures. find_library will return the first library found that matches any of the names
find_path(ZMQ_INCLUDE_DIRS zmq.h)

target_link_libraries(${PLUGIN_NAME} ${ZMQ_LIBRARIES})
target_include_directories(${PLUGIN_NAME} PRIVATE ${ZMQ_INCLUDE_DIRS})
```

### Providing libraries for Windows

Since Windows does not have standardized paths for libraries, as Linux and macOS do, it is sometimes useful to pack the appropriate Windows version of the required libraries alongside the plugin.
To do so, a _libs_ directory has to be created **at the top level** of the repository, alongside this README file, and files from all required libraries placed there. The required folder structure is:

```
    libs
    ├─ include           #library headers
    ├─ lib
        ├─ x64           #64-bit compile-time (.lib) files
        └─ x86           #32-bit compile time (.lib) files, if needed
    └─ bin
        ├─ x64           #64-bit runtime (.dll) files
        └─ x86           #32-bit runtime (.dll) files, if needed
```

DLLs in the bin directories will be copied to the open-ephys GUI _shared_ folder when installing.
