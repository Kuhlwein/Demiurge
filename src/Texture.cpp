//
// Created by kuhlwein on 5/25/19.
//

#include <imgui/examples/libs/gl3w/GL/gl3w.h>
#include <iostream>
#include <cstring>
#include <memory>
#include <zfp/include/zfp.h>
#include <chrono>
#include <thread>

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

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interp );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interp );

	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, GL_RGB,GL_UNSIGNED_BYTE, nullptr);
}

void Texture::uploadData(GLenum format, GLenum type, const GLvoid *data) {
	glActiveTexture( GL_TEXTURE0+0 );
	glBindTexture( GL_TEXTURE_2D, id );
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,width,height,format,type,data);
}

TextureData* Texture::downloadData(GLenum format,GLenum type) {
	this->bind(0);
	if (type == GL_FLOAT) {
		auto data = std::make_unique<float[]>(width*height);
		glGetTexImage( GL_TEXTURE_2D,0,format,type,data.get());

		return new TextureData(std::move(data),getWidth(),getHeight());
	}
	std::cout << "not implemented yet, returning nullpointer...\n";
	return nullptr;
}

std::unique_ptr<float[]> Texture::downloadDataRAW(GLenum format, GLenum type) {
	this->bind(0);
	if (type == GL_FLOAT) {
		auto data = std::make_unique<float[]>(width*height);

		glGetTexImage( GL_TEXTURE_2D,0,format,type,data.get());

		return std::move(data);
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

    int width = this->width;
    this->width = texture->width;
    texture->width = width;

    int height = this->height;
    this->height = texture->height;
    texture->height = height;
}

void Texture::bind(GLuint point) {
    glActiveTexture(GL_TEXTURE0+point);
    glBindTexture( GL_TEXTURE_2D, id );
}

void Texture::bind(ShaderProgram *program, GLuint point, std::string s) {
	int loc = glGetUniformLocation(program->getId(),(s=="") ? name.c_str() : s.c_str());
	if (loc>=0) {
		glActiveTexture(GL_TEXTURE0+point);
		glBindTexture( GL_TEXTURE_2D, id );
		glUniform1i(loc,point);
	}
}


TextureData::TextureData(std::unique_ptr<float[]> data, int width, int height) {
	this->height = height;
	this->width = width;
	initialized = false;

	auto f = [this](std::unique_ptr<float[]> data) {
		std::unique_lock<std::mutex> lk(mtx);

		float c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;

		std::cout << "data with size: " << (float) (this->width * this->height * sizeof(float)) / 1000000 << "M\n";

		zfp_type type = zfp_type_float;
		zfp_field *field = zfp_field_2d(data.get(), type, this->width, this->height);

		// allocate metadata for a compressed stream
		zfp_stream *zfp = zfp_stream_open(NULL);

		zfp_stream_set_accuracy(zfp, 1e-6);

		// allocate buffer for compressed data
		bufsize = zfp_stream_maximum_size(zfp, field);

		auto tmpbuffer = std::make_unique<uchar[]>(bufsize);

		// associate bit stream with allocated buffer
		bitstream *stream = stream_open(tmpbuffer.get(), bufsize);
		zfp_stream_set_bit_stream(zfp, stream);

		// compress entire array
		size_t size = zfp_compress(zfp, field);
		std::cout << "compressed size: " << (float) (size) / 1000000 << "M\n";

		buffer = new uchar[size];
		memcpy(buffer, tmpbuffer.get(), size);

		float c2 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;
		std::cout << "time: " << c2 - c1 << "\n";

		initialized = true;
		cv.notify_all();
	};

	std::thread t = std::thread(f,std::move(data));
	t.detach();
}

std::unique_ptr<float[]> TextureData::get() {
	std::unique_lock<std::mutex> lk(mtx);
	if(!initialized) cv.wait(lk);

	auto data = std::make_unique<float[]>(width*height);

	zfp_type type = zfp_type_float;
	zfp_field* field = zfp_field_2d(data.get(), type, width, height);

	// allocate metadata for a compressed stream
	zfp_stream* zfp = zfp_stream_open(NULL);

	zfp_stream_set_accuracy(zfp, 1e-6);

	// associate bit stream with allocated buffer
	bitstream* stream = stream_open(buffer, bufsize);
	zfp_stream_set_bit_stream(zfp, stream);

	zfp_stream_rewind(zfp);
	int success = zfp_decompress(zfp, field);
	std::cout << "success: " << (float)(success)/1000000 << "M\n";

	return std::move(data);
}
