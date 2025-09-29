CXX ?= g++
CXXFLAGS ?= -O3 -std=c++17 -Wall -Wextra
INCLUDES = -Iinclude
SOURCES = src/Forest.cpp src/Calibrator.cpp src/RuleGuard.cpp src/detector_main.cpp
OBJS = $(SOURCES:.cpp=.o)
all: detector_main
detector_main: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
clean:
	rm -f $(OBJS) detector_main
