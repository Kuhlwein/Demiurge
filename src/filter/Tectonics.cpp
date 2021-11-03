//
// Created by kuhlwein on 6/29/21.
//

#include <iostream>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "Project.h"
#include "Tectonics.h"



void TectonicsMenu::update_self(Project *p) {

}

std::shared_ptr<BackupFilter> TectonicsMenu::makeFilter(Project *p) {
	return std::make_shared<ProgressFilter>(p, [](Project* p){return p->get_terrain();}, std::move(std::make_unique<Tectonics>(p)));
}

TectonicsMenu::TectonicsMenu() : FilterModal("Tectonics") {

}

Tectonics::~Tectonics() {

}

Tectonics::Tectonics(Project *p) {
    plates.push_back(new Plate(p->getWidth(),p->getHeight()));
    plates.push_back(new Plate(p->getWidth(),p->getHeight()));
    plates[0]->updateRotationBy((float)0.01, glm::vec3(1, 0, 1));
    plates[1]->updateRotationBy(-(float)0.01, glm::vec3(0, 0, 1));



	Shader* shader = Shader::builder()
			.include(fragmentBase)
			.create("",R"(
	fc =  st.x>0.5 ? texture(img,st).r : 0;
)");
	setzero = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();

	setzero->bind();
	p->setCanvasUniforms(setzero);
	p->apply(setzero,plates[0]->getTexture());

	shader = Shader::builder()
            .include(fragmentBase)
            .create("",R"(
	fc =  st.x<0.5 ? texture(img,st).r : 0;
)");
    setzero = ShaderProgram::builder()
            .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
            .addShader(shader->getCode(), GL_FRAGMENT_SHADER)
            .link();

    setzero->bind();
    p->setCanvasUniforms(setzero);
    p->apply(setzero,plates[1]->getTexture());



    /*
     * SET SHADERS
     */

    shader = Shader::builder()
            .include(fragmentBase)
            .include(cornerCoords)
            .create("",R"(
	fc = 0;
)");
    setzero = ShaderProgram::builder()
            .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
            .addShader(shader->getCode(), GL_FRAGMENT_SHADER)
            .link();

    Shader* tectonicShader = Shader::builder()
            .include(fragmentBase)
            .include(cornerCoords)
            .create(R"(
uniform sampler2D plate;
uniform mat3 rotationmatrix;
uniform float plateIndex;

vec4 plateTexture(sampler2D img, vec2 st) {
    vec2 spheric = tex_to_spheric(st);
    vec3 cartesian = spheric_to_cartesian(spheric);

    cartesian = rotationmatrix*cartesian;

    spheric = cartesian_to_spheric(cartesian);
    vec2 st_ = spheric_to_tex(spheric);

	return texture(plate,st_);
}
)","");

    shader = Shader::builder()
            .include(fragmentBase)
            .include(cornerCoords)
            .include(tectonicShader)
            .create("",R"(
	fc = texture(scratch1,st).r + (plateTexture(plate,st).r>0.0 ? plateIndex : 0.0);
)");
    foldShader = ShaderProgram::builder()
            .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
            .addShader(shader->getCode(), GL_FRAGMENT_SHADER)
            .link();
}

void Tectonics::run() {
    for (int i=0; i<10000; i++) {
        for (Plate* p : plates) {
            p->rotate();
        }


        fold(setzero,foldShader);

    }

    setProgress({true,1});
}

void Tectonics::fold(ShaderProgram *zero, ShaderProgram *operation) {
    dispatchGPU([this,zero,operation](Project* p) {
        zero->bind();
        p->apply(zero,p->get_terrain());
        p->get_terrain()->swap(p->get_scratch1());

        float index = 1; //Index 0 is no plate
        for (Plate* plate : plates) {
            p->add_texture(plate->getTexture());

            operation->bind();
            plate->setRotationUniform(operation);
            int id = glGetUniformLocation(operation->getId(),"plateIndex");
            glUniform1f(id,index++);
            p->setCanvasUniforms(operation);

            p->apply(operation,p->get_terrain());
            p->remove_texture(plate->getTexture());
            p->get_terrain()->swap(p->get_scratch1());
        }

        p->get_terrain()->swap(p->get_scratch1());
    });
}

Plate::Plate(int width, int height) {
    texture = new Texture(width,height,GL_RG32F,"plate");

    rotation = glm::mat4(1);
}

Plate::~Plate() {
    delete texture;
}

Texture *Plate::getTexture() {
    return texture;
}

void Plate::rotate() {
    rotation *= rotationDirection;
}

void Plate::setRotationUniform(ShaderProgram* shaderProgram) {
    glm::mat3 a = rotation;
    int id = glGetUniformLocation(shaderProgram->getId(),"rotationmatrix");
    glUniformMatrix3fv(id,1,GL_FALSE,glm::value_ptr(a));
}

void Plate::updateRotationBy(float theta, glm::vec3 axis) {
    rotationDirection = glm::rotate(rotation,theta, axis);
}




/*
 * Ocean:
 * For each pixel, get current plate
 * Spread, gaussian-like, remember distance from divergence to plates and from plate to divergence
 * For each plate generate new crust based on distance
 *
 *
 *
 *
 */