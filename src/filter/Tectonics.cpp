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
    plate = new Plate(p->getWidth(),p->getHeight());
    //p->add_texture(plate->getTexture());


	Shader* shader = Shader::builder()
			.include(fragmentBase)
			.create("",R"(
	fc =  texture(img,st).r;
)");
	ShaderProgram* setzero = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();

	setzero->bind();
	p->setCanvasUniforms(setzero);
	p->apply(setzero,plate->getTexture());



}

void Tectonics::run() {
    for (int i=0; i<10000; i++)
    dispatchGPU([i,this](Project* p) {
        p->get_terrain()->swap(p->get_scratch1());

        Shader* shader = Shader::builder()
                .include(fragmentBase)
                .include(cornerCoords)
                .create(R"(
uniform sampler2D plate;
uniform float theta;
uniform mat3 rotationmatrix;
)",R"(
    vec2 spheric = tex_to_spheric(st);
    vec3 cartesian = spheric_to_cartesian(spheric);

    cartesian = rotationmatrix*cartesian;

    spheric = cartesian_to_spheric(cartesian);
    vec2 st_ = spheric_to_tex(spheric);

	fc =  texture(plate,st_).r;
)");
        ShaderProgram* setzero = ShaderProgram::builder()
                .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
                .addShader(shader->getCode(), GL_FRAGMENT_SHADER)
                .link();

        setzero->bind();

        int id = glGetUniformLocation(setzero->getId(), "theta");
        glUniform1f(id,i*M_PI/1000);

        plate->rotate((float)0.01, glm::vec3(1, 0, 0));

        glm::mat3 a = plate->rotation;
        id = glGetUniformLocation(setzero->getId(),"rotationmatrix");
        glUniformMatrix3fv(id,1,GL_FALSE,glm::value_ptr(a));

        id = glGetUniformLocation(setzero->getId(),"plates");
        glUniformMatrix3fv(id,1,GL_FALSE,glm::value_ptr(a));

        //id = glGetUniformLocation(setzero->getId(),"plate");
        //glBindTexture(id,plate->getTexture()->getId());
        //NEED TO BIND TO UNIT

        p->setCanvasUniforms(setzero);
        p->apply(setzero,p->get_terrain());
    });

    setProgress({true,1});
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

void Plate::rotate(float theta, glm::vec3 axis) {
    rotation = glm::rotate(rotation,theta, axis);
}
