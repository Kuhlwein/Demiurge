//
// Created by kuhlwein on 7/18/20.
//

#include "Project.h"
#include <Shader.h>
#include <iostream>
#include "Filter.h"


FreeSelectFilter::FreeSelectFilter(Project* p) {
	first_mousepos = p->getMouse();

	//Calculation shader
	Shader* shader2 = Shader::builder()
			.include(fragmentBase)
			.include(mouseLocation)
			.create("uniform vec2 mouseFirst;",R"(

vec3 AB = vec3(mouse-mousePrev,0);
vec3 BC = vec3(mousePrev-mouseFirst,0);
vec3 CA = vec3(mouseFirst-mouse,0);
vec3 AP = vec3(st-mouse,0);
vec3 BP = vec3(st-mousePrev,0);
vec3 CP = vec3(st-mouseFirst,0);

bool a = AB.x*AP.y>=AB.y*AP.x;
bool b = BC.x*BP.y>=BC.y*BP.x;
bool c = CA.x*CP.y>CA.y*CP.x;

if (a == b && b == c) {
	fc = 1.0 - texture(scratch2,vec2(st)).r;
} else {
	fc = texture(scratch2,vec2(st)).r;
}

)");

	Shader* shader = Shader::builder()
			.include(fragmentBase)
			.include(mouseLocation)
			.include(def_pi)
			.create("uniform vec2 mouseFirst;",R"(

float phi = (mouse.y*2-1)*M_PI/2;
float theta = (mouse.x*2-1)*M_PI;
vec3 A = vec3(sin(M_PI/2-phi)*cos(theta),sin(M_PI/2-phi)*sin(theta),cos(M_PI/2-phi));

phi = (mousePrev.y*2-1)*M_PI/2;
theta = (mousePrev.x*2-1)*M_PI;
vec3 B = vec3(sin(M_PI/2-phi)*cos(theta),sin(M_PI/2-phi)*sin(theta),cos(M_PI/2-phi));

phi = (mouseFirst.y*2-1)*M_PI/2;
theta = (mouseFirst.x*2-1)*M_PI;
vec3 C = vec3(sin(M_PI/2-phi)*cos(theta),sin(M_PI/2-phi)*sin(theta),cos(M_PI/2-phi));

phi = (st.y*2-1)*M_PI/2;
theta = (st.x*2-1)*M_PI;
vec3 P = vec3(sin(M_PI/2-phi)*cos(theta),sin(M_PI/2-phi)*sin(theta),cos(M_PI/2-phi));

vec3 average = (A+B+C);

vec3 a = cross(A,B);
vec3 b = cross(B,C);
vec3 c = cross(C,A);
//same sign as P!

float s = sign(dot(a,average));

if (s*dot(a,P)>0 && s*dot(b,P)>0 && s*dot(c,P)>0) {
//if(sign(dot(a,P))==sign(dot(a,average)) && sign(dot(b,P))==sign(dot(b,average)) && sign(dot(c,P))==sign(dot(c,average))) {
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
}



void FreeSelectFilter::run(Project *p) {
	ImGuiIO io = ImGui::GetIO();

	glm::vec2 texcoord = p->getMouse();
	glm::vec2 texcoordPrev = p->getMousePrev();

	bool a = texcoord != texcoordPrev;
	a &= texcoord != first_mousepos;
	a &= texcoordPrev != first_mousepos;

	if (io.MouseDown[0]) {
		if (a) {
		program->bind();
		int id = glGetUniformLocation(program->getId(), "value");
		glUniform1f(id, 1);
		id = glGetUniformLocation(program->getId(), "mouse");
		glUniform2f(id, texcoord.x, texcoord.y);
		id = glGetUniformLocation(program->getId(), "mousePrev");
		glUniform2f(id, texcoordPrev.x, texcoordPrev.y);
		id = glGetUniformLocation(program->getId(), "mouseFirst");
		glUniform2f(id, first_mousepos.x, first_mousepos.y);
		p->apply(program, p->get_scratch1());
		p->get_scratch2()->swap(p->get_scratch1());

		//float* data = (float*)p->get_terrain()->downloadData();
		//std::cout << data[0] << "\n";
	}
	} else {
		ShaderProgram *program = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(copy_img->getCode(), GL_FRAGMENT_SHADER)
				.link();
		p->apply(program,p->get_selection(),{{p->get_scratch2(),"to_be_copied"}});
		p->finalizeFilter();
	}
}

void FreeSelectFilter::finalize(Project *p) {

}

bool FreeSelectFilter::isfinished() {

}

Shader *FreeSelectFilter::getShader() {
	static Shader* s = Shader::builder()
			.create("",R"(
float overlay = (1-texture(scratch1,st_p).r)*0.25;
fc = fc*(1-overlay) + overlay*vec4(0);
)");
	return s;
}

