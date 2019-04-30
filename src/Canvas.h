//
// Created by kuhlwein on 4/30/19.
//

#ifndef DEMIURGE_CANVAS_H
#define DEMIURGE_CANVAS_H


#include "Vbo.h"

class Canvas {
public:
    Canvas(int height, int width);
    ~Canvas();
    void render();
private:
    Vbo *vbo;
};


#endif //DEMIURGE_CANVAS_H
