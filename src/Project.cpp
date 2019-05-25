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
#include "Menu.h"

//todo remove

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <imgui/imgui.h>

Project::Project(GLFWwindow* window) {
    this->window = window;


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

uniform vec2 mouse;

uniform float u_time;

void main () {
    float dx = dFdx(st.x);
    float dy = dFdy(st.y);

    float r = distance(mouse*vec2(1000,500),st*vec2(1000,500));
    if (r<5 && r>5-length(vec2(dx,dy)*vec2(1000,500))) {
        fc = texture(img, st).rrrr + 0.5;
    } else {
        fc = texture(img, st).rrrr;

    }

    float m = dFdx(st.x);



    float x1 = texture(img, st-vec2(dx,0)).r;
    float x2 = texture(img, st+vec2(dx,0)).r;
    float y1 = texture(img, st-vec2(0,dy)).r;
    float y2 = texture(img, st+vec2(0,dy)).r;

    float k = round(dx*20000);
    float test = round(mod(gl_FragCoord.x/8-gl_FragCoord.y/8+u_time,1));

    if (abs(x1-mod(x1,0.2)-(x2-mod(x2,0.2)))>0) fc = vec4(test,test,test,0);
    if (abs(y1-mod(y1,0.2)-(y2-mod(y2,0.2)))>0) fc = vec4(test,test,test,0);
}
    )";


    program = ShaderProgram::builder()
            .addShader(code,GL_VERTEX_SHADER)
            .addShader(code2,GL_FRAGMENT_SHADER)
            .link();

    int w;
    int h;
    int comp;
    std::string filename = "/home/kuhlwein/Desktop/eu.png";

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

    canvas = new Canvas(h,w,this);
    menu = new Menu(this);

}

void Project::update() {
    /*
     File
     Edit
     Select
     View
     Render
     filters
    */



    if (ImGui::BeginMainMenuBar()) {
        menu->update();

        ImGui::EndMainMenuBar();
    }
    canvas->update(program->getId());
}

void Project::render() {
    program->bind();
    canvas->render();
}

Project::~Project() {
    delete(program);
    delete(canvas);
    std::cout << "a\n";
}

int Project::getWindowWidth() {
    int width, height;
    glfwGetWindowSize(window,&width,&height);
    return width;
}

int Project::getWindowHeight() {
    int width, height;
    glfwGetWindowSize(window,&width,&height);
    return height;
}


