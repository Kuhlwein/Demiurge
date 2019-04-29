//
// Created by kuhlwein on 4/9/19.
//

#include <iostream>
#include <GL/gl3w.h>
#include <vector>
#include "Project.h"
#include "Vbo.h"
#include "ShaderProgram.h"

Project::Project() {

    std::string code =  R"(
#version 430
layout (location = 0) in vec2 vp;
layout (location = 1) in vec2 vt;
out vec2 st;

void main () {
   st = vt;
   gl_Position = vec4 (vp, 0.0, 1.0);
}
    )";

    std::string code2 =  R"(
#version 430
in vec2 st;
out vec4 fc;

void main () {
    fc = vec4(st,1,0);
}
    )";

    program = ShaderProgram::builder()
            .addShader(code,GL_VERTEX_SHADER)
            .addShader(code2,GL_FRAGMENT_SHADER)
            .link();
    
}

void Project::update(){

}

void Project::render(){

    program->bind();


    //glBindVertexArray(VAO);

    std::vector<float> positions = {
            -0.5, 0.5, 0,
            -0.5, -0.5, 0,
            0.5, -0.5, 0,
            0.5, 0.5, 0
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

    Vbo vbo(positions, textures, indices);
    vbo.render();

}

void Project::cleanup() {

}

Project::~Project() {
    std::cout << "a\n";
}
