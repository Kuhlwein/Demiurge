//
// Created by kuhlwein on 4/9/19.
//

#include <iostream>
#include <GL/gl3w.h>
#include <vector>
#include "Project.h"
#include "Vbo.h"
#include "ShaderProgram.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//todo remove

#include <glm/glm.hpp>
#include <glm/ext.hpp>

Project::Project() {

    std::string code =  R"(
#version 430

layout (location=0) in vec3 position;
layout (location=1) in vec2 texCoord;

out vec2 st;

uniform mat4 worldMatrix;
uniform mat4 projectionMatrix;

void main()
{
    gl_Position = projectionMatrix * worldMatrix * vec4(position, 1.0);
    st = texCoord;
}

    )";

    std::string code2 =  R"(
#version 430
in vec2 st;
uniform sampler2D img;
out vec4 fc;

void main () {
    float r = texture(img,st).r;
    fc = vec4(r,r,r,0);
}
    )";


    program = ShaderProgram::builder()
            .addShader(code,GL_VERTEX_SHADER)
            .addShader(code2,GL_FRAGMENT_SHADER)
            .link();

    int w;
    int h;
    int comp;
    std::string filename = "/home/kuhlwein/Desktop/heightdata.png";

    stbi_info(filename.c_str(),&w,&h,&comp);

    unsigned char* image = stbi_load(filename.c_str(), &w, &h, &comp, STBI_rgb);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    int format = GL_RED;//GL_RGBA;
    { // create the texture
        GLuint id;
        glGenTextures(1,&id);
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, id );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        // linear allows us to scale the window up retaining reasonable quality
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        // same internal format as compute shader input

        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);

        // bind to image unit so can write to specific pixels from the shader
        glBindImageTexture( 0, id, 0, false, 0, GL_WRITE_ONLY, GL_R32F );
        glBindTexture( GL_TEXTURE_2D, id );
    }

    canvas = new Canvas(h,w);

}

void Project::update(){

}

void Project::render(){
    float FOVY = glm::radians(60.0); //radian
    float TANFOV = glm::tan(FOVY *0.5);
    float aspectRatio = 1.7777;
    float Z_NEAR = 0.001f;
    float Z_FAR = 1000.f;

    glm::mat4 model(1.f);


    glm::mat4 projection = glm::perspective(FOVY,aspectRatio,Z_NEAR,Z_FAR);
    glm::mat4 world = glm::translate(model,glm::vec3(0.0,0.0,-5.0));


    program->bind();

    int id = glGetUniformLocation(program->getId(),"projectionMatrix");



    glUniformMatrix4fv(id,1,GL_FALSE,glm::value_ptr(projection));

    id = glGetUniformLocation(program->getId(),"worldMatrix");
    glUniformMatrix4fv(id,1,GL_FALSE,glm::value_ptr(world));

    canvas->render();
}

Project::~Project() {
    delete(program);
    delete(canvas);
    std::cout << "a\n";
}
