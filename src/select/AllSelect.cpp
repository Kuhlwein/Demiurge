//
// Created by kuhlwein on 7/25/20.
//

#include "Project.h"
#include "AllSelect.h"


AllSelect::AllSelect() : Modal("All", [this](Project* p) {
	return this->update_self(p);
}) {

}

bool AllSelect::update_self(Project *p) {
	p->NEW_dispatchFilter(std::move(std::make_unique<SelectAllFilter>(p)));
	return true;
}

SelectAllFilter::SelectAllFilter(Project *p) : BackupFilter(p,[](Project* p){return p->get_selection();}) {
	program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(fragment_set->getCode(), GL_FRAGMENT_SHADER)
			.link();
	program->bind();
	int id = glGetUniformLocation(program->getId(),"value");
	glUniform1f(id,1.0);
}

SelectAllFilter::~SelectAllFilter() {

}

void SelectAllFilter::run() {
	p->apply(program,p->get_selection());
	p->finalizeFilter();
}

void SelectAllFilter::finalize() {
	add_history();
}
