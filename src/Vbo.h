//
// Created by kuhlwein on 4/27/19.
//

#include <vector>
#include <GL/gl.h>


#ifndef DEMIURGE_VBO_H
#define DEMIURGE_VBO_H


class Vbo {
public:
    void render();
    Vbo(std::vector<float> positions,std::vector<float> texCoords, std::vector<int> indices);
    ~Vbo();
private:
    GLuint vaoId;
    int vertexCount;
};


#endif //DEMIURGE_VBO_H
