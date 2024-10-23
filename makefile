# Compiler and flags
CXX = g++
CXXFLAGS = -I. -I./SoLoud -DWITH_MINIAUDIO -static-libstdc++ -static-libgcc -static

# Object files for SoLoud (auto-generated for each .cpp and .c file)
SOLOUD_OBJS = $(patsubst %.cpp,%.o,$(wildcard SoLoud/*.cpp)) \
			  $(patsubst %.c,%.o,$(wildcard SoLoud/*.c))

# Main target
all: main

# Build main executable
main: main.o $(SOLOUD_OBJS)
	$(CXX) main.o $(SOLOUD_OBJS) -o main

# Compile main.o
main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp

# Compile SoLoud object files
SoLoud/%.o: SoLoud/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

SoLoud/%.o: SoLoud/%.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
cleanWin:
	del /Q main main.exe *.o SoLoud/*.o


# Clean up build files
clean:
	ifeq ($(OS),Windows_NT)
		del /Q main.exe *.o SoLoud/*.o
	else
		rm -f main *.o SoLoud/*.o
	endif