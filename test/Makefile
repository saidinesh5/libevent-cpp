#### PROJECT SETTINGS ####
# Compiler used
CXX ?= g++
# Extension of source files used in the project
SRC_EXT = cc
# Path to the source directory, relative to the makefile
SRC_PATH = .
# Path to the build directory, relative to the makefile
BUILD_PATH = .
# Path to the base directory
BASE_PATH = $(PWD)/..
# Space-separated pkg-config libraries used by this project
LIBS = -levent++ -L$(BASE_PATH)
# General compiler flags
COMPILE_FLAGS = -std=c++14 -Wall -Wextra -g -Wno-unused-parameter
# Add additional include paths
INCLUDES = -I$(BASE_PATH)/src/event -I$(BASE_PATH)/src/http -I$(BASE_PATH)/src/util -I$(BASE_PATH)/src/thread -I$(BASE_PATH)/src/core
# General linker settings
LINK_FLAGS = 
#### END PROJECT SETTINGS ####

SOURCES = $(shell find $(SRC_PATH) -name '*.$(SRC_EXT)')
BINS = $(SOURCES:$(SRC_PATH)/%.$(SRC_EXT)=$(BUILD_PATH)/%)

% : %.cc
	$(CXX) -o $@ $< $(COMPILE_FLAGS) $(LIBS) $(INCLUDES)

all: $(BINS)

clean:
	rm $(BINS)
	rm *.log