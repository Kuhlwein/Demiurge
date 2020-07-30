//
// Created by kuhlwein on 7/31/20.
//

#include "Project.h"
#include <imgui/imgui.h>
#include "ScaleMenu.h"

ScaleMenu::ScaleMenu() : InstantFilterModal("Scale (Multiply)") {}

void ScaleMenu::update_self(Project *p) {
	if(ImGui::DragFloat("Value", &scale, 0.01f/10, 0, 0, "%.4f", 1.0f)) {
		filter->run();
	}
}

std::unique_ptr<BackupFilter> ScaleMenu::makeFilter(Project *p) {
	return std::move(std::make_unique<ScaleFilter>(p,&scale));
}

ScaleFilter::ScaleFilter(Project *p, float* scale) : BackupFilter(p, [](Project* p){return p->get_terrain();}) {
	this->scale = scale;
	Shader* shader = Shader::builder()
			.include(fragmentBase)
			.create("uniform float scale;",R"(
	fc = texture(img,vec2(st)).r* (1+(scale-1)*texture(sel,vec2(st)).r);
)");

	program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();
}

void ScaleFilter::run() {
	restoreBackup();

	program->bind();
	int id = glGetUniformLocation(program->getId(), "scale");
	glUniform1f(id,*scale);

	p->apply(program, p->get_scratch1());
	p->get_scratch1()->swap(p->get_terrain());

}

bool ScaleFilter::isFinished() {
	return false;
}

ScaleFilter::~ScaleFilter() {

}