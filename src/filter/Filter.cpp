//
// Created by kuhlwein on 7/18/20.
//

#include "Project.h"
#include <Shader.h>
#include <iostream>
#include "Filter.h"


FreeSelectFilter::FreeSelectFilter(Project* p) {
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
		p->getGeometry()->setup_triangle(program,first_mousepos,texcoord,texcoordPrev);

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

