//
// Created by kuhlwein on 7/18/20.
//

#include "Project.h"
#include <Shader.h>
#include <bits/unique_ptr.h>
#include <iostream>
#include "FreeSelect.h"
#include "selection.h"


FreeSelect::FreeSelect() : Modal("Free select", [this](Project* p) {
	return this->update_self(p);
}) {
	filter = std::make_shared<NoneFilter>();
}

bool FreeSelect::update_self(Project *p) {
	ImGuiIO io = ImGui::GetIO();

	Shader* mode = selection::selection_mode();

	if(ImGui::Button("Close")) {
		return true;
	}

	filter->run();

	bool a = ImGui::IsAnyWindowHovered() || ImGui::IsAnyItemHovered();
	if(!a && io.MouseDown[0] && io.MouseDownDuration[0]==0) {
		filter = std::make_shared<FreeSelectFilter>(p, mode);
		p->dispatchFilter(filter);
	}
	if (filter->isFinished()) filter = std::make_shared<NoneFilter>();

	return false;
}

FreeSelectFilter::FreeSelectFilter(Project *p, Shader* mode) : BackupFilter(p,[](Project* p) {return p->get_selection();}) {
	first_mousepos = p->getMouse();
	this->mode = mode;

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
	if (finished) return;

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

		}
	} else {
		Shader* shader = Shader::builder()
				.include(fragmentBase)
				.include(p->getGeometry()->offset())
				.include(mode)
				.create("",R"(
float a = 0;
a += texture(scratch2,offset(st, vec2(1,0), textureSize(img,0))).r;
a += texture(scratch2,offset(st, vec2(-1,0), textureSize(img,0))).r;
a += texture(scratch2,offset(st, vec2(0,1), textureSize(img,0))).r;
a += texture(scratch2,offset(st, vec2(0,-1), textureSize(img,0))).r;

float val = a==0 ? 0 : texture(scratch2, st).r;
val = a==4 ? 1 : val;

fc = selection_mode(texture(sel,st).r,val);
)");

		ShaderProgram *program = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
				.link();
		p->apply(program,p->get_scratch1());
		p->get_scratch1()->swap(p->get_selection());

		add_history();
		p->finalizeFilter();
		finished = true;
	}
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

bool FreeSelectFilter::isFinished() {
	return finished;
}
