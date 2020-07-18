//
// Created by kuhlwein on 7/18/20.
//


#include <gl3w/GL/gl3w.h>
#include <ShaderProgram.h>
#include <Shader.h>
#include <iostream>
#include "FreeSelectModal.h"
#include "Project.h"

FreeSelectModal::FreeSelectModal() : Modal("Free select", [this](Project* p) {
	return this->update_self(p);
}) {
	Shader* shader = Shader::builder()
			.include(fragmentBase)
			.include(mouseLocation)
			.create("uniform vec2 mouseFirst;",R"(
float x1 = mouse.x;
float x2 = mousePrev.x;
float x3 = mouseFirst.x;

float y1 = mouse.y;
float y2 = mousePrev.y;
float y3 = mouseFirst.y;

float x = st.x;
float y = st.y;

float denominator = (x1*(y2 - y3) + y1*(x3 - x2) + x2*y3 - y2*x3);
float t1 = (x*(y3 - y1) + y*(x1 - x3) - x1*y3 + y1*x3) / denominator;
float t2 = (x*(y2 - y1) + y*(x1 - x2) - x1*y2 + y1*x2) / (-denominator);
float s = t1 + t2;

if(t1 >= 0 && t1<=1 && t2 >=0 && t2<=1 && s<=1) {
	fc = 1.0 - texture(sel,vec2(st)).r;
} else {
	fc = texture(sel,vec2(st)).r;
}


)");
	program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();
}

bool FreeSelectModal::update_self(Project *p) {
	ImGuiIO io = ImGui::GetIO();

	glm::vec2 texcoord = p->getMouse();
	glm::vec2 texcoordPrev = p->getMousePrev();


	if(io.MouseDown[0] && first_mouseclick) {
		//First
		first_mousepos = texcoord;
		first_mouseclick = false;
	} else if (io.MouseDown[0]){
		//Middle

		program->bind();
		int id = glGetUniformLocation(program->getId(),"value");
		glUniform1f(id,1);
		id = glGetUniformLocation(program->getId(),"mouse");
		glUniform2f(id,texcoord.x,texcoord.y);
		id = glGetUniformLocation(program->getId(),"mousePrev");
		glUniform2f(id,texcoordPrev.x,texcoordPrev.y);
		id = glGetUniformLocation(program->getId(),"mouseFirst");
		glUniform2f(id,first_mousepos.x,first_mousepos.y);
		p->apply(program,p->get_scratch1());
		p->get_selection()->swap(p->get_scratch1());

	} else if (!first_mouseclick) {
		//Last
		first_mouseclick = true;
	}




	if(ImGui::Button("Ok")) {
		return true;
	}


	return false;
}

