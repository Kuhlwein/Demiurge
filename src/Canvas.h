//
// Created by kuhlwein on 4/30/19.
//

#ifndef DEMIURGE_CANVAS_H
#define DEMIURGE_CANVAS_H


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

    float canvasAspect;
};


#endif //DEMIURGE_CANVAS_H
