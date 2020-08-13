//
// Created by kuhlwein on 7/18/20.
//

#include "Project.h"
#include <Shader.h>
#include <iostream>
#include <glm/glm/ext.hpp>
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

	if(ImGui::Button("Apply")) {
		return true;
	}

	filter->run(p);

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

	Shader* triangle_shader = Shader::builder()
			.include(fragmentBase)
			.include(cornerCoords)
			.include(mouseLocation)
			.include(def_pi)
			.create(R"(
uniform vec3 a;
uniform vec3 b;
uniform vec3 c;
uniform float s;

bool free_select() {
	vec3 P = spheric_to_cartesian(tex_to_spheric(st)).xyz;
	return s*dot(a,P)>0 && s*dot(b,P)>0 && s*dot(c,P)>0;
}
)","");

	Shader* shader = Shader::builder()
			.include(triangle_shader)
			.create("",R"(
if (free_select()) {
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

void FreeSelectFilter::run(Project* p) {
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

			//TODO setup triangle
			//p->getGeometry()->setup_triangle(program,first_mousepos,texcoord,texcoordPrev);
			{
				int id = glGetUniformLocation(program->getId(), "mouse");
				glUniform2f(id, texcoord.x, texcoord.y);
				id = glGetUniformLocation(program->getId(), "mousePrev");
				glUniform2f(id, texcoordPrev.x, texcoordPrev.y);
				id = glGetUniformLocation(program->getId(), "mouseFirst");
				glUniform2f(id, first_mousepos.x, first_mousepos.y);

				auto cornerCoords = p->getCoords();
				auto f = [cornerCoords](glm::vec2 p) {
					p.x = (p.x*(cornerCoords[3]-cornerCoords[2])+cornerCoords[2]);
					p.y = (p.y*(cornerCoords[1]-cornerCoords[0])+cornerCoords[0]);
					return glm::vec3(cos(p.y)*cos(p.x),cos(p.y)*sin(p.x),sin(p.y));
				};

				auto A = f(texcoord);
				auto B = f(texcoordPrev);
				auto C = f(first_mousepos);
				auto a = cross(A,B);
				auto b = cross(B,C);
				auto c = cross(C,A);
				id = glGetUniformLocation(program->getId(), "a");
				glUniform3fv(id, 1,glm::value_ptr(a));
				id = glGetUniformLocation(program->getId(), "b");
				glUniform3fv(id, 1,glm::value_ptr(b));
				id = glGetUniformLocation(program->getId(), "c");
				glUniform3fv(id, 1,glm::value_ptr(c));
				auto average = (A+B+C);
				float s = glm::sign(dot(a,average));
				id = glGetUniformLocation(program->getId(), "s");
				glUniform1f(id,s);

				p->setCanvasUniforms(program);
			}

			p->apply(program, p->get_scratch1());
			p->get_scratch2()->swap(p->get_scratch1());

		}
	} else {
		Shader* shader = Shader::builder()
				.include(fragmentBase)
				.include(cornerCoords)
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
		program->bind();
		p->setCanvasUniforms(program);
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
