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

		Shader* s = Shader::builder()
				.create("",R"(
fc = fc*0.75 + 0.25 * texture(scratch1,st);
)");
		p->setFilterView(true,s);
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
		p->get_scratch2()->swap(p->get_scratch1());

	} else if (!first_mouseclick) {
		//Last
		p->setFilterView(false, nullptr);
		first_mouseclick = true;
	}




	if(ImGui::Button("Ok")) {
		return true;
	}


	return false;
}

