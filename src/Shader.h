//
// Created by kuhlwein on 6/13/19.
//

#ifndef DEMIURGE_SHADER_H
#define DEMIURGE_SHADER_H


#include <string>
#include <vector>



class Shader {
public:
    class builder;
    Shader(std::string code, std::vector<Shader*> includes);
    //void include(Shader* shader);
    std::string getCode();
private:
    std::string code;
    std::vector<Shader*> includes;
};

class Shader::builder {
public:
    builder include(Shader* shader);
    Shader* create(std::string code);
private:
    std::vector<Shader*> includes;
};

Shader* vertexSetup = Shader::builder()
        .create(R"(
layout (location=0) in vec3 position;
layout (location=1) in vec2 texCoord;

out vec2 st;

uniform mat4 worldMatrix;
uniform mat4 projectionMatrix;
)");

Shader* vertex3D = Shader::builder()
        .include(vertexSetup)
        .create(R"(
gl_Position = projectionMatrix * worldMatrix * vec4(position, 1.0);
st = texCoord;
)");

Shader* vertex2D = Shader::builder()
        .include(vertexSetup)
        .create(R"(
gl_Position =  vec4(position, 1.0);
st = texCoord;
)");










#endif //DEMIURGE_SHADER_H
