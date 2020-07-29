//
// Created by kuhlwein on 5/25/19.
//

#include <GL/gl3w.h>
#include <iostream>
#include <cstring>
#include "Texture.h"

Texture::~Texture() {
	glDeleteTextures(1,&id);
}

Texture::Texture(int width, int height, GLenum format, std::string name, GLint interp) {
    internalformat = format;
    this->name = name;
    this->width = width;
    this->height = height;

    glGenTextures(1,&id);
    glActiveTexture( GL_TEXTURE0+0 );
    glBindTexture( GL_TEXTURE_2D, id );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    // linear allows us to scale the window up retaining reasonable quality
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interp );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interp );
    // same internal format as compute shader input

	//auto data = new unsigned char[width * height * 3];
    //for (int i=0; i<width*height*3; i++) data[i]=0;
	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, GL_RGB,GL_UNSIGNED_BYTE, nullptr);
	//glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, 0,0, nullptr);
	//delete[] data;

    // bind to image unit so can write to specific pixels from the shader
    //glBindImageTexture( unit, id, 0, false, 0, GL_READ_WRITE,  format); //todo: for compute shader???
    //glActiveTexture(GL_TEXTURE0+unit);
    //glBindTexture( GL_TEXTURE_2D, id );
}

void Texture::uploadData(GLenum format, GLenum type, const GLvoid *data) {
	glActiveTexture( GL_TEXTURE0+0 );
	glBindTexture( GL_TEXTURE_2D, id );
    glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, data);
	//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, type, data);
}

GLvoid *Texture::downloadData(GLenum format,GLenum type) {
	this->bind(0);
	if (type == GL_FLOAT) {
		float* data = new float[width*height];
		//glGetTexImage( GL_TEXTURE_2D,0,format,type,data);




		static GLuint pbo;

		glGenBuffers(1, &pbo);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
		glBufferData(GL_PIXEL_PACK_BUFFER, getWidth() * getHeight() * 4, NULL, GL_STREAM_READ);


		glActiveTexture(GL_TEXTURE0);
		bind(0);
		//glBindTexture(GL_TEXTURE_2D, texture);

		glGetTexImage(GL_TEXTURE_2D,
					  0,
					  format,
					  type,
					  nullptr);


		glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
		float* mappedBuffer = (float*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);

		memcpy(data,mappedBuffer,getWidth() * getHeight()*4);

		//now mapped buffer contains the pixel data

		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);


		return data;
	}
	std::cout << "not implemented yet, returning nullpointer...\n";
	return nullptr;
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

void Texture::bind(ShaderProgram *program, GLuint point, std::string s) {
	int loc = glGetUniformLocation(program->getId(),(s=="") ? name.c_str() : s.c_str());
	//std::cout << name << " " << s  << " " << loc << "\n";
	if (loc>=0) {
		glActiveTexture(GL_TEXTURE0+point);
		glBindTexture( GL_TEXTURE_2D, id );
		glUniform1i(loc,point);
	}
}





