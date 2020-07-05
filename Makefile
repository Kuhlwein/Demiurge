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
GL3W = $(LIB)/gl3w
glfw = $(LIB)/glfw
glm = $(LIB)/glm

EXE = example_glfw_opengl3.out
SOURCES = src/main.cpp
SOURCES += src/Project.cpp src/Vbo.cpp src/ShaderProgram.cpp src/projections/Canvas.cpp src/projections/Orthographic.cpp src/projections/Mollweide.cpp src/projections/Mercator.cpp src/projections/Equiretangular.cpp src/Shader.cpp src/imgui/imgui_color_gradient.cpp src/Texture.cpp src/Menu.cpp src/UndoHistory.cpp src/menus/selection.cpp src/menus/view.cpp src/menus/edit.cpp
SOURCES += $(IMGUI)/imgui_impl_glfw.cpp $(IMGUI)/imgui_impl_opengl3.cpp
SOURCES += $(IMGUI)/imgui.cpp $(IMGUI)/imgui_demo.cpp $(IMGUI)/imgui_draw.cpp $(IMGUI)/imgui_widgets.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
UNAME_S := $(shell uname -s)

##---------------------------------------------------------------------
## OPENGL LOADER
##---------------------------------------------------------------------

## Using OpenGL loader: gl3w [default]
SOURCES += ../libs/gl3w/GL/gl3w.c
CXXFLAGS = -I../libs/gl3w

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
	LIBS = -lGL `pkg-config --static --libs glfw3`

	CXXFLAGS += -I$(IMGUI) -I$(GL3W) -I$(glfw)  `pkg-config --cflags glfw3` -I$(glm) -Ilib
	CXXFLAGS += -Wall -Wformat
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
   LIBS = -static -lglfw3 -lgdi32 -lopengl32 -limm32

   CXXFLAGS += -I$(IMGUI) -I$(GL3W) -I$(glfw)  `pkg-config --cflags glfw3` -I$(glm) -Ilib
   CXXFLAGS += -Wall -Wformat
   CFLAGS = $(CXXFLAGS)
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $< -I src/menus/ -I src/projections/ -I src/

%.o:src/menus/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $< -I src/menus/ -I src/projections/ -I src/

%.o:src/projections/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $< -I src/menus/ -I src/projections/ -I src/

%.o:src/imgui/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(GL3W)/GL/%.c
# %.o:../libs/glad/src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)
	./$(EXE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)
