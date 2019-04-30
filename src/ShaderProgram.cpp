//
// Created by kuhlwein on 4/29/19.
//

#include <GL/gl3w.h>
#include <iostream>
#include "ShaderProgram.h"

ShaderProgram::ShaderProgram(int id) {
    this->id = id;
}

void ShaderProgram::bind() {
    glUseProgram(id);
}

int ShaderProgram::getId() {
    return id;
}


ShaderProgram * ShaderProgram::builder::link() {
    GLint status;
    int programId = glCreateProgram();
    if (programId==0) std::cout << "could not create program\n";

    for(int shaderId : shaders) glAttachShader(programId,shaderId);
    glLinkProgram(programId);

    glValidateProgram(programId);
    glGetProgramiv(programId, GL_LINK_STATUS,&status);
    if(status == GL_FALSE) std::cout << "Error linking program\n";

    for(int shaderId : shaders) {
        glDetachShader(programId,shaderId);
        glDeleteShader(shaderId);
    }

    glValidateProgram(programId);
    glGetProgramiv(programId, GL_VALIDATE_STATUS,&status);
    if(status == GL_FALSE) std::cout << "Error validating program\n";

    return new ShaderProgram(programId);
}

ShaderProgram::builder& ShaderProgram::builder::addShader(std::string shader, GLenum shadertype) {
    int shaderid = glCreateShader(shadertype);
    shaders.push_back(shaderid);
    if (shaderid==0) std::cout << "Error creating shader\n";

    GLchar const* files[] = { shader.c_str() };
    GLint lengths[]       = { static_cast<GLint>(shader.size()) };

    glShaderSource(shaderid,1,files,lengths);
    glCompileShader(shaderid);

    GLint success = 0;
    glGetShaderiv(shaderid, GL_COMPILE_STATUS, &success);
    if (success==GL_FALSE) std::cout << "ERROR compiling shader!\n";

    return *this;
}
