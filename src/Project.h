//
// Created by kuhlwein on 4/9/19.
//

#ifndef DEMIURGE_PROJECT_H
#define DEMIURGE_PROJECT_H
#include <GL/gl3w.h>
#include <glfw/include/GLFW/glfw3.h>
#include "ShaderProgram.h"
#include "Vbo.h"
#include "Canvas.h"
#include "Texture.h"
#include "Window.h"


class Project {
    public:
    ShaderProgram *program;
    Canvas *canvas;
    Project(GLFWwindow* window);
    ~Project();
    void update();
    void render();
    int getWindowWidth();
    int getWindowHeight();
    void brush(float x, float y);
private:
    GLFWwindow* window;
    Window* window1;
    Texture* terrain;
    Texture* scratchPad;
    Texture* selection;
    Vbo* vbo;
};


#endif //DEMIURGE_PROJECT_H
