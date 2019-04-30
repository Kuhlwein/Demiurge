//
// Created by kuhlwein on 4/9/19.
//

#ifndef DEMIURGE_PROJECT_H
#define DEMIURGE_PROJECT_H
#include <GL/gl3w.h>
#include "ShaderProgram.h"
#include "Vbo.h"
#include "Canvas.h"


class Project {
    public:
    ShaderProgram *program;
    Canvas *canvas;
    Project();
    ~Project();
    void update();
    void render();
};


#endif //DEMIURGE_PROJECT_H
