#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#
# You will need GLFW (http://www.glfw.org):
# Linux:
#   apt-get install libglfw-dev
# Mac OS X:
#   brew install glfw
# MSYS2:
#   pacman -S --noconfirm --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-glfw
#

#CXX = g++
#CXX = clang++

LIB = lib
IMGUI = $(LIB)/imgui
GL3W = $(LIB)/imgui/examples/libs/gl3w
glfw = $(LIB)/imgui/examples/libs
glm = $(LIB)/glm
zfp = $(LIB)/zfp

EXE = example_glfw_opengl3.out
SOURCES = src/main.cpp
SOURCES = $(IMGUI)/examples/imgui_impl_glfw.cpp $(IMGUI)/examples/examples/imgui_impl_opengl3.cpp
SOURCES += $(wildcard src/*.cpp) $(wildcard src/*/*.cpp)  $(wildcard $(IMGUI)/*.cpp)
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
UNAME_S := $(shell uname -s)

##---------------------------------------------------------------------
## OPENGL LOADER
##---------------------------------------------------------------------

## Using OpenGL loader: gl3w [default]
SOURCES += $(GL3W)/GL/gl3w.c
CXXFLAGS = -I$(GL3W) -DIMGUI_IMPL_OPENGL_LOADER_GL3W

## Using OpenGL loader: glew
## (This assumes a system-wide installation)
# CXXFLAGS = -lGLEW -DIMGUI_IMPL_OPENGL_LOADER_GLEW

## Using OpenGL loader: glad
## (You'll also need to change the rule at line ~77 of this Makefile to compile/link glad.c/.o)
# SOURCES += ../libs/glad/src/glad.c
# CXXFLAGS = -I../libs/glad/include -DIMGUI_IMPL_OPENGL_LOADER_GLAD

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS = -lGL `pkg-config --static --libs glfw3` -L$(zfp)/lib -lzfp

	CXXFLAGS += -I$(IMGUI) -I$(GL3W) -I$(glfw)  `pkg-config --cflags glfw3` -I$(glm) -Ilib -I$(zfp)/array -I$(zfp)/include -O3
	CXXFLAGS += -Wall -Wformat -g
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS = -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
	#LIBS += -L/usr/local/lib -lglfw3
	LIBS += -L/usr/local/lib -lglfw

	CXXFLAGS += -I../ -I../../ -I/usr/local/include
	CXXFLAGS += -Wall -Wformat
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
   ECHO_MESSAGE = "Windows"
   LIBS = -static -lglfw3 -lgdi32 -lopengl32 -limm32 -L$(zfp)/lib -lzfp

   CXXFLAGS += -I$(IMGUI) -I$(GL3W) -I$(glfw)  `pkg-config --cflags glfw3` -I$(glm) -Ilib -I$(zfp)/array -I$(zfp)/include
   CXXFLAGS += -Wall -Wformat
   CFLAGS = $(CXXFLAGS)
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $< -I src/

%.o:src/*/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $< -I src/

%.o:$(IMGUI)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI)/examples/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(GL3W)/GL/%.c
# %.o:../libs/glad/src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)
	./$(EXE)

$(EXE): $(OBJS) lib/zfp/lib/libzfp.a
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)

lib/zfp/lib/libzfp.a :
	$(MAKE) -C lib/zfp
