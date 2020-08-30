//
// Created by kuhlwein on 8/23/20.
//

#include "Project.h"
#include <imgui/imgui.h>
#include "HeightSelection.h"
#include "selection.h"

bool HeightSelection::update_self(Project *p) {
	Shader* mode = selection::selection_mode();
	if(first) {
		filter = std::make_shared<HeightSelectFilter>(p, mode);
		p->dispatchFilter(filter);
		first = false;
	}

	ImGuiIO io = ImGui::GetIO();
	ImGui::DragFloatRange2("Range",range,range+1,0.01);
	filter->setRange(range[0],range[1]);
	filter->setMode(mode);

	filter->run(p);

	if(ImGui::Button("Apply")) {
		filter->finish();
		filter.reset();
		first = true;
		return true;
	}

	return false;
}

HeightSelection::HeightSelection() : Modal("Height",[this](Project* p){return this->update_self(p);}) {

}

HeightSelectFilter::HeightSelectFilter(Project *p, Shader *mode) : BackupFilter(p, [](Project* p) {return p->get_selection();}) {
	this->mode = mode;

	Shader* shader = Shader::builder()
			.include(fragmentBase)
			.create("uniform float lower; uniform float upper;",R"(
	float h = texture(img,st).r;
	fc = (h<=upper && h>=lower) ? 1 : 0;
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
	int id = glGetUniformLocation(program->getId(),"lower");
	glUniform1f(id,lower);
	id = glGetUniformLocation(program->getId(),"upper");
	glUniform1f(id,upper);
	p->apply(program,p->get_scratch1());
	p->get_scratch1()->swap(p->get_scratch2());
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

void HeightSelectFilter::setRange(float min, float max) {
	lower = min;
	upper = max;
}

void HeightSelectFilter::setMode(Shader *shader) {
	mode = shader;
}

void HeightSelectFilter::finish() {
	Shader* shader = Shader::builder()
			.include(fragmentBase)
			.include(mode)
			.create("",R"(
	float val = texture(scratch1, st).r;
	fc = selection_mode(texture(sel,st).r,val);
)");

	ShaderProgram *program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();
	program->bind();
	p->apply(program,p->get_scratch2());
	p->get_scratch2()->swap(p->get_selection());

	add_history();
	p->finalizeFilter();
	finished = true;
}
