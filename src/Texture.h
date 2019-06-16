//
// Created by kuhlwein on 5/25/19.
//

#ifndef DEMIURGE_TEXTURE_H
#define DEMIURGE_TEXTURE_H


#include <GL/gl.h>

class Texture {
public:
    Texture(int width, int height, GLenum format, GLuint unit);
    void uploadData(GLenum format, GLenum type, const GLvoid * data);
    GLuint getId();
    int getWidth();
    int getHeight();
    void swap(Texture* texture);
    void bind(GLuint point);
private:
    GLuint id;
    GLint internalformat;
    int width;
    int height;
};


#endif //DEMIURGE_TEXTURE_H
