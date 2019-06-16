//
// Created by kuhlwein on 5/25/19.
//

#include <GL/gl3w.h>
#include "Texture.h"

Texture::Texture(int width, int height, GLenum format, GLuint unit) {
    internalformat = format;
    this->width = width;
    this->height = height;

    glGenTextures(1,&id);
    glActiveTexture( GL_TEXTURE0+unit );
    glBindTexture( GL_TEXTURE_2D, id );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    // linear allows us to scale the window up retaining reasonable quality
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    // same internal format as compute shader input

    glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, 0, 0, nullptr);

    // bind to image unit so can write to specific pixels from the shader
    //glBindImageTexture( unit, id, 0, false, 0, GL_READ_WRITE,  format); //todo: for compute shader???
    //glActiveTexture(GL_TEXTURE0+unit);
    //glBindTexture( GL_TEXTURE_2D, id );
}

void Texture::uploadData(GLenum format, GLenum type, const GLvoid *data) {
    glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, data);
}

GLuint Texture::getId() {
    return id;
}

int Texture::getWidth() {
    return width;
}

int Texture::getHeight() {
    return height;
}

void Texture::swap(Texture* texture) {
    int id = this->id;
    this->id = texture->id;
    texture->id = id;
}

void Texture::bind(GLuint point) {
    glActiveTexture(GL_TEXTURE0+point);
    glBindTexture( GL_TEXTURE_2D, id );
}
