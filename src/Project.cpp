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
#include "Texture.h"

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
layout(binding=0) uniform sampler2D img;
layout(binding=1) uniform sampler2D sel;
out vec4 fc;

uniform vec2 mouse;

uniform float u_time;

void main () {
    float dx = dFdx(st.x);
    float dy = dFdy(st.y);

    float r = distance(mouse*vec2(1000,500),st*vec2(1000,500));
    if (r<20 && r>20-length(vec2(dx,dy)*vec2(1000,500))) {
        fc = texture(img, st).rrrr + 0.5;
    } else {
        fc = texture(img, st).rrrr;

    }

    float m = dFdx(st.x);



    float x1 = texture(sel, st-vec2(dx,0)).r;
    float x2 = texture(sel, st+vec2(dx,0)).r;
    float y1 = texture(sel, st-vec2(0,dy)).r;
    float y2 = texture(sel, st+vec2(0,dy)).r;

    float k = round(dx*20000);
    float test = round(mod(gl_FragCoord.x/8-gl_FragCoord.y/8+u_time,1));

    if (abs(x1-mod(x1,0.2)-(x2-mod(x2,0.2)))>0) fc = vec4(test,test,test,0);
    if (abs(y1-mod(y1,0.2)-(y2-mod(y2,0.2)))>0) fc = vec4(test,test,test,0);
}
    )";

    std::string code3 =  R"(
#version 430
in vec2 st;
layout(binding=0) uniform sampler2D img;

out vec4 fc;



void main () {
        fc = texture(img, st).rrrr + 0.1;
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

    terrain = new Texture(w,h,GL_R32F,0);
    terrain->uploadData(GL_RGB,GL_UNSIGNED_BYTE,image);

    //TODO something
    scratchPad = new Texture(w,h,GL_R32F,0);
    scratchPad->uploadData(GL_RGB,GL_UNSIGNED_BYTE,image);

    GLuint fbo;
    glGenFramebuffers(1,&fbo);
    glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,terrain->getId(),0);
    std::cout << fbo << "\n";

    GLuint fbo2;
    glGenFramebuffers(1,&fbo2);
    glBindFramebuffer(GL_FRAMEBUFFER,fbo2);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,scratchPad->getId(),0);
    std::cout << fbo2 << "\n";

    glBindFramebuffer(GL_FRAMEBUFFER,0);

    std::vector<float> positions = {
            -1, -1, 0,
            -1, 1, 0,
            1, 1, 0,
            1, -1, 0
    };
    std::vector<float> textures = {
            0.0, 0.0,
            0.0, 1,
            1, 1,
            1, 0
    };
    std::vector<int> indices = {
            0, 1, 3,
            2, 3, 1
    };
    vbo = new Vbo(positions, textures, indices);

    //TODO

    selection = new Texture(w,h,GL_R32F,1);
    filename = "/home/kuhlwein/Desktop/eu_sel.png";
    stbi_info(filename.c_str(),&w,&h,&comp);
    image = stbi_load(filename.c_str(), &w, &h, &comp, STBI_rgb);
    selection->uploadData(GL_RGB,GL_UNSIGNED_BYTE,image);

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

    ImGuiIO io = ImGui::GetIO();
    static bool hej = false;
    if(io.MouseDown[0] && hej) {
        ShaderProgram *program2 = ShaderProgram::builder()
                .addShader(R"(
#version 430

layout (location=0) in vec3 position;
layout (location=1) in vec2 texCoord;

out vec2 st;

void main()
{
    gl_Position =  vec4(position, 1.0);
    st = texCoord;
}
    )", GL_VERTEX_SHADER)
                .addShader(R"(
#version 430
in vec2 st;
layout(binding=0) uniform sampler2D img;
layout(binding=1) uniform sampler2D sel;
out float fc;

void main () {
    fc = texture(img, st).r + 0.01;
}
    )", GL_FRAGMENT_SHADER)
                .link();

        glActiveTexture(GL_TEXTURE0);
        glBindFramebuffer(GL_FRAMEBUFFER, 1);
        glViewport(0, 0, terrain->getWidth(), terrain->getHeight());
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,scratchPad->getId(),0);
        glBindTexture(GL_TEXTURE_2D, terrain->getId());
        program2->bind();

        vbo->render();

        scratchPad->swap(terrain);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        hej = false;
    } else if (true) hej = true;


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


