//
// Created by kuhlwein on 4/30/19.
//

#ifndef DEMIURGE_CANVAS_H
#define DEMIURGE_CANVAS_H


#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include "Vbo.h"

class Project;

class Canvas {
public:
    Canvas(int height, int width, Project* project);
    ~Canvas();
    void render();
    void update(int id);
private:
    Vbo *vbo;
    float x, y, z;
    void pan(float dx, float dy);
    Project* project;

    float FOVY; //radian
    float TANFOV;
    float windowAspect;
    int windowWidth;
    int windowHeight;
    float Z_NEAR;
    float Z_FAR;
    float ZOOM;
    glm::vec2 lastMouse;

    float canvasAspect;

    glm::vec2 mousePos(ImVec2 pos);
};


#endif //DEMIURGE_CANVAS_H
