//
// Created by kuhlwein on 4/27/19.
//

#include <GL/gl3w.h>
#include "Vbo.h"

Vbo::Vbo(std::vector<float> positions, std::vector<float> texCoords, std::vector<int> indices) {
    vertexCount = indices.size();

    glGenVertexArrays(1,&vaoId);
    glBindVertexArray(vaoId);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof (float), positions.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT, GL_FALSE,0, nullptr);
    glEnableVertexAttribArray(0);

    GLuint vbo2;
    glGenBuffers(1, &vbo2);
    glBindBuffer(GL_ARRAY_BUFFER, vbo2);
    glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof (float), texCoords.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1,2,GL_FLOAT, GL_FALSE,0, nullptr);
    glEnableVertexAttribArray(1);

    GLuint vbo3;
    glGenBuffers(1, &vbo3);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vbo3);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size()*sizeof (int),indices.data(),GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void Vbo::render() {
    glBindVertexArray(vaoId);
    glDrawElements(GL_TRIANGLES,vertexCount,GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

Vbo::~Vbo() {

}







