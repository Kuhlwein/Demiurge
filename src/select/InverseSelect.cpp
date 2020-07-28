//
// Created by kuhlwein on 7/25/20.
//

#include "Project.h"
#include "InverseSelect.h"
#include "Shader.h"

std::function<Texture *(Project *p)> InverseSelect::targetGetter() {
	return [](Project* p){return p->get_selection();};
}

void InverseSelect::filter(Project *p) {
	Shader* shader = Shader::builder()
			.include(fragmentBase)
			.create(R"(
)",R"(
fc = 1-texture(sel,st).r;
)");

	auto program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();
	program->bind();
	int id = glGetUniformLocation(program->getId(),"value");
	glUniform1f(id,1.0);

	p->apply(program,p->get_scratch1());
	p->get_scratch1()->swap(p->get_selection());
}

InverseSelect::InverseSelect() : FilterMenu("Inverse") {

}
