//
// Created by kuhlwein on 1/15/22.
//


#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "Project.h"
#include "Plate.h"

Plate::Plate(int width, int height) {
    texture = new Texture(width,height,GL_RGBA32F,"plate");

    rotation = glm::mat4(1);
    angularVelocity = glm::vec3(0);
}

Plate::~Plate() {
    delete texture;
}

Texture *Plate::getTexture() {
    return texture;
}

void Plate::rotate() {
    rotation = glm::rotate(rotation,glm::length(angularVelocity), glm::normalize(angularVelocity));
}

void Plate::setPlateUniforms(ShaderProgram* shaderProgram, int index) {
    int id = glGetUniformLocation(shaderProgram->getId(),"plateIndex");
    glUniform1f(id,index);

    glm::mat3 a = rotation;
    id = glGetUniformLocation(shaderProgram->getId(),"rotationmatrix");
    glUniformMatrix3fv(id,1,GL_FALSE,glm::value_ptr(a));

    glm::mat3 b = glm::transpose(a);
    id = glGetUniformLocation(shaderProgram->getId(),"inverserotationmatrix");
    glUniformMatrix3fv(id,1,GL_FALSE,glm::value_ptr(b));

    id = glGetUniformLocation(shaderProgram->getId(),"plateVelocity");
    glUniform3f(id,angularVelocity.x,angularVelocity.y,angularVelocity.z);
}

void Plate::updateRotationBy(float theta, glm::vec3 axis) {
    angularVelocity += theta*axis;
}



/*
 * plateID (PLATE elevation/age/direction)
 */



/*
 * Index, movement-direction, distance
 */
/*
 * Ocean:
 * For each pixel, get current plate
 * Spread, gaussian-like, remember distance from divergence to plates and from plate to divergence
 * For each plate generate new crust based on distance
 *
 *
 *
 *
 */