//
// Created by kuhlwein on 7/25/20.
//

#include "Project.h"
#include "InverseSelect.h"
#include "Shader.h"


InverseSelect::InverseSelect() : Modal("Inverse", [this](Project* p) {
	return this->update_self(p);
}) {

}

bool InverseSelect::update_self(Project *p) {
	auto filter = std::make_unique<SelectInverseFilter>(p);
	filter->run();
	return true;
}

SelectInverseFilter::SelectInverseFilter(Project *p) : BackupFilter(p,[](Project* p){return p->get_selection();}) {
	Shader* shader = Shader::builder()
			.include(fragmentBase)
			.create(R"(
)",R"(
fc = 1-texture(sel,st).r;
)");

	program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();
	program->bind();
	int id = glGetUniformLocation(program->getId(),"value");
	glUniform1f(id,1.0);
}

SelectInverseFilter::~SelectInverseFilter() {

}

void SelectInverseFilter::run() {
	p->apply(program,p->get_scratch1());
	p->get_scratch1()->swap(p->get_selection());
	add_history();
	p->finalizeFilter();
}

bool SelectInverseFilter::isFinished() {
	return false;
}

//void SelectInverseFilter::finalize() {
//	add_history();
//}
