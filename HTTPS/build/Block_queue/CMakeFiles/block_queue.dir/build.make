# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.6

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/hy/c++/HTTPS

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/hy/c++/HTTPS/build

# Include any dependencies generated for this target.
include Block_queue/CMakeFiles/block_queue.dir/depend.make

# Include the progress variables for this target.
include Block_queue/CMakeFiles/block_queue.dir/progress.make

# Include the compile flags for this target's objects.
include Block_queue/CMakeFiles/block_queue.dir/flags.make

Block_queue/CMakeFiles/block_queue.dir/Block_queue.cpp.o: Block_queue/CMakeFiles/block_queue.dir/flags.make
Block_queue/CMakeFiles/block_queue.dir/Block_queue.cpp.o: ../Block_queue/Block_queue.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/hy/c++/HTTPS/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object Block_queue/CMakeFiles/block_queue.dir/Block_queue.cpp.o"
	cd /home/hy/c++/HTTPS/build/Block_queue && /usr/bin/g++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/block_queue.dir/Block_queue.cpp.o -c /home/hy/c++/HTTPS/Block_queue/Block_queue.cpp

Block_queue/CMakeFiles/block_queue.dir/Block_queue.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/block_queue.dir/Block_queue.cpp.i"
	cd /home/hy/c++/HTTPS/build/Block_queue && /usr/bin/g++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/hy/c++/HTTPS/Block_queue/Block_queue.cpp > CMakeFiles/block_queue.dir/Block_queue.cpp.i

Block_queue/CMakeFiles/block_queue.dir/Block_queue.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/block_queue.dir/Block_queue.cpp.s"
	cd /home/hy/c++/HTTPS/build/Block_queue && /usr/bin/g++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/hy/c++/HTTPS/Block_queue/Block_queue.cpp -o CMakeFiles/block_queue.dir/Block_queue.cpp.s

Block_queue/CMakeFiles/block_queue.dir/Block_queue.cpp.o.requires:

.PHONY : Block_queue/CMakeFiles/block_queue.dir/Block_queue.cpp.o.requires

Block_queue/CMakeFiles/block_queue.dir/Block_queue.cpp.o.provides: Block_queue/CMakeFiles/block_queue.dir/Block_queue.cpp.o.requires
	$(MAKE) -f Block_queue/CMakeFiles/block_queue.dir/build.make Block_queue/CMakeFiles/block_queue.dir/Block_queue.cpp.o.provides.build
.PHONY : Block_queue/CMakeFiles/block_queue.dir/Block_queue.cpp.o.provides

Block_queue/CMakeFiles/block_queue.dir/Block_queue.cpp.o.provides.build: Block_queue/CMakeFiles/block_queue.dir/Block_queue.cpp.o


# Object files for target block_queue
block_queue_OBJECTS = \
"CMakeFiles/block_queue.dir/Block_queue.cpp.o"

# External object files for target block_queue
block_queue_EXTERNAL_OBJECTS =

Block_queue/libblock_queue.a: Block_queue/CMakeFiles/block_queue.dir/Block_queue.cpp.o
Block_queue/libblock_queue.a: Block_queue/CMakeFiles/block_queue.dir/build.make
Block_queue/libblock_queue.a: Block_queue/CMakeFiles/block_queue.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/hy/c++/HTTPS/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libblock_queue.a"
	cd /home/hy/c++/HTTPS/build/Block_queue && $(CMAKE_COMMAND) -P CMakeFiles/block_queue.dir/cmake_clean_target.cmake
	cd /home/hy/c++/HTTPS/build/Block_queue && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/block_queue.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
Block_queue/CMakeFiles/block_queue.dir/build: Block_queue/libblock_queue.a

.PHONY : Block_queue/CMakeFiles/block_queue.dir/build

Block_queue/CMakeFiles/block_queue.dir/requires: Block_queue/CMakeFiles/block_queue.dir/Block_queue.cpp.o.requires

.PHONY : Block_queue/CMakeFiles/block_queue.dir/requires

Block_queue/CMakeFiles/block_queue.dir/clean:
	cd /home/hy/c++/HTTPS/build/Block_queue && $(CMAKE_COMMAND) -P CMakeFiles/block_queue.dir/cmake_clean.cmake
.PHONY : Block_queue/CMakeFiles/block_queue.dir/clean

Block_queue/CMakeFiles/block_queue.dir/depend:
	cd /home/hy/c++/HTTPS/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/hy/c++/HTTPS /home/hy/c++/HTTPS/Block_queue /home/hy/c++/HTTPS/build /home/hy/c++/HTTPS/build/Block_queue /home/hy/c++/HTTPS/build/Block_queue/CMakeFiles/block_queue.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : Block_queue/CMakeFiles/block_queue.dir/depend

