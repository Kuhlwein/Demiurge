//
// Created by kuhlwein on 8/23/20.
//

#include "Project.h"
#include <imgui/imgui.h>
#include "HeightSelection.h"
#include "selection.h"

void HeightSelection::update_self(Project *p) {
	ImGuiIO io = ImGui::GetIO();

	mode = selection::selection_mode();
}

std::shared_ptr<BackupFilter> HeightSelection::makeFilter(Project *p) {
	return std::make_shared<HeightSelectFilter>(p,mode);
}

HeightSelection::HeightSelection() : FilterModal("Height") {

}

HeightSelectFilter::HeightSelectFilter(Project *p, Shader *mode) : BackupFilter(p, [](Project* p) {return p->get_selection();}) {
	this->mode = mode;

	Shader* shader = Shader::builder()
			.include(fragmentBase)
			.create("",R"(
	fc = 1.0 - texture(scratch2,vec2(st)).r;
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
	delete program;
}

HeightSelectFilter::~HeightSelectFilter() {

}

void HeightSelectFilter::run(Project *p) {
	if (finished) return;
	program->bind();
	p->apply(program,p->get_scratch2());
}

Shader *HeightSelectFilter::getShader() {
	static Shader* s = Shader::builder()
			.create("",R"(
float overlay = (1-texture(scratch1,st_p).r)*0.25;
fc = fc*(1-overlay) + overlay*vec4(0);
)");
	return s;
}

bool HeightSelectFilter::isFinished() {
	return finished;
}
