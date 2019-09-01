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

/*
 *  Vertex shaders
 */
static Shader* vertexSetup = Shader::builder()
        .create(R"(
layout (location=0) in vec3 position;
layout (location=1) in vec2 texCoord;

out vec2 st;

uniform mat4 worldMatrix;
uniform mat4 projectionMatrix;
)");

static Shader* vertex3D = Shader::builder()
        .include(vertexSetup)
        .create(R"(
gl_Position = projectionMatrix * worldMatrix * vec4(position, 1.0);
st = texCoord;
)");

static Shader* vertex2D = Shader::builder()
        .include(vertexSetup)
        .create(R"(
gl_Position =  vec4(position, 1.0);
st = texCoord;
)");


/*
 * Fragment shaders
 */

static Shader* fragmentBase = Shader::builder()
        .create(R"(
in vec2 st;
layout(binding=0) uniform sampler2D img;
layout(binding=1) uniform sampler2D sel;
out float fc;
)");

static Shader* mouseLocation = Shader::builder()
        .create(R"(
uniform vec2 mouse;
uniform vec2 mousePrev;
)");

static Shader* fragmentCopy = Shader::builder()
        .include(fragmentBase)
        .create(R"(
fc = texture(img, st).r;
)");

static Shader* fragmentClear = Shader::builder()
        .include(fragmentBase)
        .create(R"(
fc = 0;
)");







#endif //DEMIURGE_SHADER_H
