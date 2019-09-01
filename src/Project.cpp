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
#include "Texture.h"
#include "Shader.h"


//todo remove

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <imgui/imgui.h>

Project::Project(GLFWwindow* window) {
    ImGui::GetIO().IniFilename = NULL;

    this->window = window;


    std::string code = vertex3D->getCode();

    std::string code2 =  R"(
#version 430
in vec2 st;
layout(binding=0) uniform sampler2D img;
layout(binding=1) uniform sampler2D sel;
out vec4 fc;

uniform vec2 mouse;
uniform float fisk;

uniform float u_time;


float dx = dFdx(st.x);
float dy = dFdy(st.y);


void draw_brush_outline(inout vec4 fc) {
    float r = distance(mouse*vec2(5001,2500),st*vec2(5001,2500));
    if (r<20 && r>20-1.5*length(vec2(dx,dy)*vec2(5001,2500))) {
        vec2 test = mouse*vec2(5001,2500)-st*vec2(5001,2500);
        float k = round(mod(atan(test.y,test.x)/(2*3.1415)*1/dx/1000,1));
        fc = vec4(1,1,1,0);
    }
}

void main () {



    fc = texture(img, st).rrrr;
    float omega = atan(st.y-0.5,st.x-0.5); // -pi:pi
    float theta = length((st-0.5)*vec2(1,0.5));

    float L = 4;
    float R = 1;
    float phi = asin(L*sin(theta)/R)-theta;

    vec2 n = vec2(phi*cos(omega)/2+0.5,phi*sin(omega)/2+0.5);

    //fc = vec4(phi,phi,phi,0);
    //fc = texture(img, vec2(omega/3.14/2+0.5,phi/3.14)).rrrr;

    draw_brush_outline(fc);





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
    scratchPad2 = new Texture(w,h,GL_R32F,2);
    scratchPad2->uploadData(GL_RGB,GL_UNSIGNED_BYTE,image);

    scratchPad = new Texture(w,h,GL_R32F,2);
    scratchPad->uploadData(GL_RGB,GL_UNSIGNED_BYTE,image);



    GLuint fbo;
    glGenFramebuffers(1,&fbo);
    glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,terrain->getId(),0);
    std::cout << fbo << "\n";


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
    //menu = new Menu(this);
    window1 = new Window("test",testnamespace::test);

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

    if(ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File",true)) {
            if (ImGui::BeginMenu("hej",true)) {
                window1->menu();
                ImGui::EndMenu();
            }


            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    window1->update(this);

    canvas->update(program->getId());
}

void Project::render() {

    program->bind();
    terrain->bind(0);
    selection->bind(1);
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

void Project::brush(glm::vec2 pos, glm::vec2 prev) {
    Shader* brusher = Shader::builder()
            .include(fragmentBase)
            .include(mouseLocation)
            .create(R"(
    vec2 dims = vec2(5001,2500);
    if(distance(st*dims,mouse*dims)<20) {
      fc = texture(img, st).r + 1;
    } else {
      fc = texture(img, st).r;
    }
)");

    ShaderProgram *program2 = ShaderProgram::builder()
            .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
            .addShader(brusher->getCode(), GL_FRAGMENT_SHADER)
            .link();

    program2->bind();
    int id = glGetUniformLocation(program2->getId(),"mouse");
    //std::cout << x << "\n";
    glUniform2f(id,pos.x,pos.y);
    id = glGetUniformLocation(program2->getId(),"mousePrev");
    //std::cout << x << "\n";
    glUniform2f(id,prev.x,prev.y);

    scratchPad2->bind(0);
    apply(program2,terrain);
    terrain->swap(scratchPad2);

    ShaderProgram *program3 = ShaderProgram::builder()
            .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
            .addShader(R"(
#version 430
in vec2 st;
layout(binding=0) uniform sampler2D img;
layout(binding=1) uniform sampler2D sel;
out float fc;

uniform vec2 mouse;
uniform vec2 mousePrev;

void main () {
    fc = texture(img, st).r + min(texture(sel,st).r,0.3);
}
    )", GL_FRAGMENT_SHADER)
            .link();

    scratchPad2->bind(1);
    scratchPad->bind(0);
    apply(program3,terrain);

    //scratchPad->swap(terrain);

    //clearbrush();

}

void Project::apply(ShaderProgram *program, Texture *texture) {
    glBindFramebuffer(GL_FRAMEBUFFER, 1);
    glViewport(0, 0, texture->getWidth(), texture->getHeight());
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture->getId(),0);
    program->bind();

    vbo->render();


    glBindFramebuffer(GL_FRAMEBUFFER, 0);




    //unsigned char* image;
    //glReadPixels(0,0,5001,2500,GL_R32F,GL_UNSIGNED_BYTE,image);

}

void Project::clearbrush() {
    terrain->bind(0);
    ShaderProgram *program = ShaderProgram::builder()
            .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
            .addShader(fragmentCopy->getCode(), GL_FRAGMENT_SHADER)
            .link();
    apply(program,scratchPad);

    ShaderProgram *program2 = ShaderProgram::builder()
            .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
            .addShader(fragmentClear->getCode(), GL_FRAGMENT_SHADER)
            .link();
    apply(program2,scratchPad2);
    std::cout << "hej\n";
}





