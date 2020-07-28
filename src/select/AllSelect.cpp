//
// Created by kuhlwein on 7/25/20.
//

#include "Project.h"
#include "AllSelect.h"

std::function<Texture *(Project *p)> AllSelect::targetGetter() {
	return [](Project* p){return p->get_selection();};
}

void AllSelect::filter(Project *p) {
	auto program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(fragment_set->getCode(), GL_FRAGMENT_SHADER)
			.link();
	program->bind();
	int id = glGetUniformLocation(program->getId(),"value");
	glUniform1f(id,1.0);

	p->apply(program,p->get_selection());
}

AllSelect::AllSelect() : FilterMenu("All") {

}
