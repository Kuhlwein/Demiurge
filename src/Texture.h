//
// Created by kuhlwein on 5/25/19.
//

#ifndef DEMIURGE_TEXTURE_H
#define DEMIURGE_TEXTURE_H


#include <GL/gl.h>
#include <string>
#include "ShaderProgram.h"

class Texture {
public:

	Texture(int width, int height, GLenum format, std::string name, GLint interp=GL_NEAREST);
	~Texture();

	void uploadData(GLenum format, GLenum type, const GLvoid * data);
	GLvoid* downloadData(GLenum format=GL_RED, GLenum type=GL_FLOAT);
    GLuint getId();
    int getWidth();
    int getHeight();
    void swap(Texture* texture);
    void bind(GLuint point);
    void bind(ShaderProgram* program, GLuint point,std::string s="");
private:
    GLuint id;
    GLint internalformat;
    int width;
    int height;
    std::string name;
};


#endif //DEMIURGE_TEXTURE_H
