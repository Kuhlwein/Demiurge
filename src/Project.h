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
#include "Menu.h"


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
private:
    GLFWwindow* window;
    Menu* menu;
};


#endif //DEMIURGE_PROJECT_H
