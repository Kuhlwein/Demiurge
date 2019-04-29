//
// Created by kuhlwein on 4/29/19.
//

#ifndef DEMIURGE_SHADERPROGRAM_H
#define DEMIURGE_SHADERPROGRAM_H


#include <string>
#include <GL/gl.h>
#include <vector>

class ShaderProgram {
public:
    class builder;
    ShaderProgram(int id);
    void bind();
private:
    int id;

};

class ShaderProgram::builder {
public:
    builder& addShader(std::string shader, GLenum shadertype);
    ShaderProgram * link();
private:
    std::vector<int> shaders;
};



#endif //DEMIURGE_SHADERPROGRAM_H
