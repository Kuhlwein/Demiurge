//
// Created by kuhlwein on 7/18/20.
//

#include "Project.h"
#include <Shader.h>
#include <bits/unique_ptr.h>
#include <iostream>
#include "FreeSelectModal.h"


FreeSelectModal::FreeSelectModal() : Modal("Free select", [this](Project* p) {
	return this->update_self(p);
}) {

}

bool FreeSelectModal::update_self(Project *p) {
	ImGuiIO io = ImGui::GetIO();

	if(ImGui::Button("Ok")) {
		return true;
	}

	bool a = ImGui::IsAnyWindowHovered() || ImGui::IsAnyItemHovered();
	if(!a && io.MouseDown[0] && io.MouseDownDuration[0]==0) {
		p->NEW_dispatchFilter(std::move(std::make_unique<FreeSelectFilter>(p)));
	}

	return false;
}

FreeSelectFilter::FreeSelectFilter(Project *p) : BackupFilter(p,[](Project* p) {return p->get_selection();}) {
	first_mousepos = p->getMouse();


	Shader* shader = Shader::builder()
			.include(p->getGeometry()->triangle())
			.create("",R"(
if (free_select(mouseFirst,mouse,mousePrev)) {
	fc = 1.0 - texture(scratch2,vec2(st)).r;
} else {
	fc = texture(scratch2,vec2(st)).r;
}
)");

	program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();

	//INIT
	ShaderProgram *program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(fragment_set->getCode(), GL_FRAGMENT_SHADER)
			.link();
	program->bind();
	int id = glGetUniformLocation(program->getId(),"value");
	glUniform1f(id,0);
	p->apply(program,p->get_scratch2());
	delete program;
}

void FreeSelectFilter::run() {
	ImGuiIO io = ImGui::GetIO();

	glm::vec2 texcoord = p->getMouse();
	glm::vec2 texcoordPrev = p->getMousePrev();

	bool a = texcoord != texcoordPrev;
	a &= texcoord != first_mousepos;
	a &= texcoordPrev != first_mousepos;

	if (io.MouseDown[0]) {
		if (a) {
			program->bind();
			p->getGeometry()->setup_triangle(program,first_mousepos,texcoord,texcoordPrev);

			p->apply(program, p->get_scratch1());
			p->get_scratch2()->swap(p->get_scratch1());

			//float* data = (float*)p->get_terrain()->downloadData();
			//std::cout << data[0] << "\n";
		}
	} else {
		p->finalizeFilter();
	}
}

void FreeSelectFilter::finalize() {
	Shader* shader = Shader::builder()
			.include(fragmentBase)
			.include(p->getGeometry()->offset())
			.create("",R"(
float a = 0;
a += texture(scratch2,offset(st, vec2(1,0), textureSize(img,0))).r;
a += texture(scratch2,offset(st, vec2(-1,0), textureSize(img,0))).r;
a += texture(scratch2,offset(st, vec2(0,1), textureSize(img,0))).r;
a += texture(scratch2,offset(st, vec2(0,-1), textureSize(img,0))).r;

fc = a==0 ? 0 : texture(scratch2, st).r;
fc = a==4 ? 1 : fc;
)");



	ShaderProgram *program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();
	p->apply(program,p->get_selection());

	add_history();
}

Shader *FreeSelectFilter::getShader() {
	static Shader* s = Shader::builder()
			.create("",R"(
float overlay = (1-texture(scratch1,st_p).r)*0.25;
fc = fc*(1-overlay) + overlay*vec4(0);
)");
	return s;
}

FreeSelectFilter::~FreeSelectFilter() {
	std::cout << "Deleted from memmory!\n";
}
