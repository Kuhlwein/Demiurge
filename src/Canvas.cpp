//
// Created by kuhlwein on 4/30/19.
//

#include "Canvas.h"


Canvas::Canvas(int height, int width) {

    float aspect = (float) width / height;

    std::vector<float> positions = {
            -aspect, 1, 0,
            -aspect, -1, 0,
            aspect, -1, 0,
            aspect, 1, 0
    };
    std::vector<float> textures = {
            0.0, 0.0,
            0.0, 1,
            1, 1,
            1, 0
    };
    std::vector<int> indices = {
            0, 1, 3,
            2, 3, 1
    };

    vbo = new Vbo(positions, textures, indices);
}

void Canvas::render() {
    vbo->render();
}

Canvas::~Canvas() {
    delete(vbo);
}
