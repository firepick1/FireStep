# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

# Default target executed when no arguments are given to make.
default_target: all
.PHONY : default_target

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/karl/dev/FireStep

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/karl/dev/FireStep

#=============================================================================
# Targets provided globally by CMake.

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running interactive CMake command-line interface..."
	/usr/bin/cmake -i .
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache
.PHONY : edit_cache/fast

# Special rule for the target package
package: preinstall
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Run CPack packaging tool..."
	/usr/bin/cpack --config ./CPackConfig.cmake
.PHONY : package

# Special rule for the target package
package/fast: package
.PHONY : package/fast

# Special rule for the target package_source
package_source:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Run CPack packaging tool for source..."
	/usr/bin/cpack --config ./CPackSourceConfig.cmake /home/karl/dev/FireStep/CPackSourceConfig.cmake
.PHONY : package_source

# Special rule for the target package_source
package_source/fast: package_source
.PHONY : package_source/fast

# Special rule for the target rebuild_cache
rebuild_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake to regenerate build system..."
	/usr/bin/cmake -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache
.PHONY : rebuild_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /home/karl/dev/FireStep/CMakeFiles /home/karl/dev/FireStep/CMakeFiles/progress.marks
	$(MAKE) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /home/karl/dev/FireStep/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean
.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named test

# Build rule for target.
test: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 test
.PHONY : test

# fast build rule for target.
test/fast:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/build
.PHONY : test/fast

FireStep/JsonCommand.o: FireStep/JsonCommand.cpp.o
.PHONY : FireStep/JsonCommand.o

# target to build an object file
FireStep/JsonCommand.cpp.o:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/FireStep/JsonCommand.cpp.o
.PHONY : FireStep/JsonCommand.cpp.o

FireStep/JsonCommand.i: FireStep/JsonCommand.cpp.i
.PHONY : FireStep/JsonCommand.i

# target to preprocess a source file
FireStep/JsonCommand.cpp.i:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/FireStep/JsonCommand.cpp.i
.PHONY : FireStep/JsonCommand.cpp.i

FireStep/JsonCommand.s: FireStep/JsonCommand.cpp.s
.PHONY : FireStep/JsonCommand.s

# target to generate assembly for a file
FireStep/JsonCommand.cpp.s:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/FireStep/JsonCommand.cpp.s
.PHONY : FireStep/JsonCommand.cpp.s

FireStep/JsonController.o: FireStep/JsonController.cpp.o
.PHONY : FireStep/JsonController.o

# target to build an object file
FireStep/JsonController.cpp.o:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/FireStep/JsonController.cpp.o
.PHONY : FireStep/JsonController.cpp.o

FireStep/JsonController.i: FireStep/JsonController.cpp.i
.PHONY : FireStep/JsonController.i

# target to preprocess a source file
FireStep/JsonController.cpp.i:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/FireStep/JsonController.cpp.i
.PHONY : FireStep/JsonController.cpp.i

FireStep/JsonController.s: FireStep/JsonController.cpp.s
.PHONY : FireStep/JsonController.s

# target to generate assembly for a file
FireStep/JsonController.cpp.s:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/FireStep/JsonController.cpp.s
.PHONY : FireStep/JsonController.cpp.s

FireStep/Machine.o: FireStep/Machine.cpp.o
.PHONY : FireStep/Machine.o

# target to build an object file
FireStep/Machine.cpp.o:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/FireStep/Machine.cpp.o
.PHONY : FireStep/Machine.cpp.o

FireStep/Machine.i: FireStep/Machine.cpp.i
.PHONY : FireStep/Machine.i

# target to preprocess a source file
FireStep/Machine.cpp.i:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/FireStep/Machine.cpp.i
.PHONY : FireStep/Machine.cpp.i

FireStep/Machine.s: FireStep/Machine.cpp.s
.PHONY : FireStep/Machine.s

# target to generate assembly for a file
FireStep/Machine.cpp.s:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/FireStep/Machine.cpp.s
.PHONY : FireStep/Machine.cpp.s

FireStep/MachineThread.o: FireStep/MachineThread.cpp.o
.PHONY : FireStep/MachineThread.o

# target to build an object file
FireStep/MachineThread.cpp.o:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/FireStep/MachineThread.cpp.o
.PHONY : FireStep/MachineThread.cpp.o

FireStep/MachineThread.i: FireStep/MachineThread.cpp.i
.PHONY : FireStep/MachineThread.i

# target to preprocess a source file
FireStep/MachineThread.cpp.i:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/FireStep/MachineThread.cpp.i
.PHONY : FireStep/MachineThread.cpp.i

FireStep/MachineThread.s: FireStep/MachineThread.cpp.s
.PHONY : FireStep/MachineThread.s

# target to generate assembly for a file
FireStep/MachineThread.cpp.s:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/FireStep/MachineThread.cpp.s
.PHONY : FireStep/MachineThread.cpp.s

FireStep/NeoPixel.o: FireStep/NeoPixel.cpp.o
.PHONY : FireStep/NeoPixel.o

# target to build an object file
FireStep/NeoPixel.cpp.o:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/FireStep/NeoPixel.cpp.o
.PHONY : FireStep/NeoPixel.cpp.o

FireStep/NeoPixel.i: FireStep/NeoPixel.cpp.i
.PHONY : FireStep/NeoPixel.i

# target to preprocess a source file
FireStep/NeoPixel.cpp.i:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/FireStep/NeoPixel.cpp.i
.PHONY : FireStep/NeoPixel.cpp.i

FireStep/NeoPixel.s: FireStep/NeoPixel.cpp.s
.PHONY : FireStep/NeoPixel.s

# target to generate assembly for a file
FireStep/NeoPixel.cpp.s:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/FireStep/NeoPixel.cpp.s
.PHONY : FireStep/NeoPixel.cpp.s

FireStep/Stroke.o: FireStep/Stroke.cpp.o
.PHONY : FireStep/Stroke.o

# target to build an object file
FireStep/Stroke.cpp.o:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/FireStep/Stroke.cpp.o
.PHONY : FireStep/Stroke.cpp.o

FireStep/Stroke.i: FireStep/Stroke.cpp.i
.PHONY : FireStep/Stroke.i

# target to preprocess a source file
FireStep/Stroke.cpp.i:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/FireStep/Stroke.cpp.i
.PHONY : FireStep/Stroke.cpp.i

FireStep/Stroke.s: FireStep/Stroke.cpp.s
.PHONY : FireStep/Stroke.s

# target to generate assembly for a file
FireStep/Stroke.cpp.s:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/FireStep/Stroke.cpp.s
.PHONY : FireStep/Stroke.cpp.s

FireStep/Thread.o: FireStep/Thread.cpp.o
.PHONY : FireStep/Thread.o

# target to build an object file
FireStep/Thread.cpp.o:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/FireStep/Thread.cpp.o
.PHONY : FireStep/Thread.cpp.o

FireStep/Thread.i: FireStep/Thread.cpp.i
.PHONY : FireStep/Thread.i

# target to preprocess a source file
FireStep/Thread.cpp.i:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/FireStep/Thread.cpp.i
.PHONY : FireStep/Thread.cpp.i

FireStep/Thread.s: FireStep/Thread.cpp.s
.PHONY : FireStep/Thread.s

# target to generate assembly for a file
FireStep/Thread.cpp.s:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/FireStep/Thread.cpp.s
.PHONY : FireStep/Thread.cpp.s

test/FireLog.o: test/FireLog.cpp.o
.PHONY : test/FireLog.o

# target to build an object file
test/FireLog.cpp.o:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/test/FireLog.cpp.o
.PHONY : test/FireLog.cpp.o

test/FireLog.i: test/FireLog.cpp.i
.PHONY : test/FireLog.i

# target to preprocess a source file
test/FireLog.cpp.i:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/test/FireLog.cpp.i
.PHONY : test/FireLog.cpp.i

test/FireLog.s: test/FireLog.cpp.s
.PHONY : test/FireLog.s

# target to generate assembly for a file
test/FireLog.cpp.s:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/test/FireLog.cpp.s
.PHONY : test/FireLog.cpp.s

test/MockDuino.o: test/MockDuino.cpp.o
.PHONY : test/MockDuino.o

# target to build an object file
test/MockDuino.cpp.o:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/test/MockDuino.cpp.o
.PHONY : test/MockDuino.cpp.o

test/MockDuino.i: test/MockDuino.cpp.i
.PHONY : test/MockDuino.i

# target to preprocess a source file
test/MockDuino.cpp.i:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/test/MockDuino.cpp.i
.PHONY : test/MockDuino.cpp.i

test/MockDuino.s: test/MockDuino.cpp.s
.PHONY : test/MockDuino.s

# target to generate assembly for a file
test/MockDuino.cpp.s:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/test/MockDuino.cpp.s
.PHONY : test/MockDuino.cpp.s

test/test.o: test/test.cpp.o
.PHONY : test/test.o

# target to build an object file
test/test.cpp.o:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/test/test.cpp.o
.PHONY : test/test.cpp.o

test/test.i: test/test.cpp.i
.PHONY : test/test.i

# target to preprocess a source file
test/test.cpp.i:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/test/test.cpp.i
.PHONY : test/test.cpp.i

test/test.s: test/test.cpp.s
.PHONY : test/test.s

# target to generate assembly for a file
test/test.cpp.s:
	$(MAKE) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/test/test.cpp.s
.PHONY : test/test.cpp.s

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... edit_cache"
	@echo "... package"
	@echo "... package_source"
	@echo "... rebuild_cache"
	@echo "... test"
	@echo "... FireStep/JsonCommand.o"
	@echo "... FireStep/JsonCommand.i"
	@echo "... FireStep/JsonCommand.s"
	@echo "... FireStep/JsonController.o"
	@echo "... FireStep/JsonController.i"
	@echo "... FireStep/JsonController.s"
	@echo "... FireStep/Machine.o"
	@echo "... FireStep/Machine.i"
	@echo "... FireStep/Machine.s"
	@echo "... FireStep/MachineThread.o"
	@echo "... FireStep/MachineThread.i"
	@echo "... FireStep/MachineThread.s"
	@echo "... FireStep/NeoPixel.o"
	@echo "... FireStep/NeoPixel.i"
	@echo "... FireStep/NeoPixel.s"
	@echo "... FireStep/Stroke.o"
	@echo "... FireStep/Stroke.i"
	@echo "... FireStep/Stroke.s"
	@echo "... FireStep/Thread.o"
	@echo "... FireStep/Thread.i"
	@echo "... FireStep/Thread.s"
	@echo "... test/FireLog.o"
	@echo "... test/FireLog.i"
	@echo "... test/FireLog.s"
	@echo "... test/MockDuino.o"
	@echo "... test/MockDuino.i"
	@echo "... test/MockDuino.s"
	@echo "... test/test.o"
	@echo "... test/test.i"
	@echo "... test/test.s"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system

