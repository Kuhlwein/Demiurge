//
// Created by kuhlwein on 7/30/20.
//

#include "Project.h"
#include <imgui/imgui.h>
#include "OffsetMenu.h"

OffsetMenu::OffsetMenu() : InstantFilterModal("Offset (Add)") {}

void OffsetMenu::update_self(Project *p) {
	if(ImGui::DragFloat("Value", &offset, 0.01f/10, 0, 0, "%.4f", 1.0f)) {
		filter->run(p);
	}
}

std::unique_ptr<BackupFilter> OffsetMenu::makeFilter(Project *p) {
	return std::move(std::make_unique<OffsetFilter>(p,&offset));
}

OffsetFilter::OffsetFilter(Project *p, float* offset) : BackupFilter(p, [](Project* p){return p->get_terrain();}) {
	this->offset = offset;
	Shader* shader = Shader::builder()
			.include(fragmentBase)
			.create("uniform float offset;",R"(
	fc = texture(img,vec2(st)).r + offset*texture(sel,vec2(st)).r;
)");

	program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();
}

void OffsetFilter::run(Project* p) {
	restoreBackup();

	program->bind();
	int id = glGetUniformLocation(program->getId(), "offset");
	glUniform1f(id,*offset);

	p->apply(program, p->get_scratch1());
	p->get_scratch1()->swap(p->get_terrain());

}

bool OffsetFilter::isFinished() {
	return false;
}

OffsetFilter::~OffsetFilter() {

}
