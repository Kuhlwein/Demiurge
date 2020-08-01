//
// Created by kuhlwein on 7/18/20.
//

#include "Project.h"
#include <Shader.h>
#include <iostream>
#include "Filter.h"
#include "Texture.h"

BackupFilter::BackupFilter(Project *p, std::function<Texture *(Project *p)> target) : Filter(p) {
	this->target = target;
	std::cout << "creating backup\n";
	tmp = new Texture(p->getWidth(),p->getHeight(),GL_R32F,"tmp");

	ShaderProgram *program_backup = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(copy_img->getCode(), GL_FRAGMENT_SHADER)
			.link();
	p->apply(program_backup,tmp,{{target(p),"to_be_copied"}});
}

BackupFilter::~BackupFilter() {
	delete (tmp);
}

void BackupFilter::add_history() {
	Shader *img_tmp_diff = Shader::builder()
			.include(fragmentBase)
			.create("uniform sampler2D tmp; uniform sampler2D target;", R"(
fc = texture(tmp,st).r - texture(target, st).r;
)");
	// find difference between backup and target
	ShaderProgram *program2 = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(img_tmp_diff->getCode(), GL_FRAGMENT_SHADER)
			.link();
	p->add_texture(tmp);
	p->apply(program2, p->get_scratch1(),{{target(p),"target"}});
	p->remove_texture(tmp);
	TextureData* data = p->get_scratch1()->downloadData();

	auto h = new SnapshotHistory(data,target);
	p->add_history(h);
}

void BackupFilter::restoreUnselected() {
	Shader* shader = Shader::builder()
			.include(fragmentBase)
			.create("uniform sampler2D to_be_copied; uniform sampler2D tmp;",R"(
{
float s = texture(sel,st).r;
fc = s*texture(to_be_copied, st).r + (1-s)*texture(tmp, st).r;
}
)");
	ShaderProgram *program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();
	p->add_texture(tmp);
	p->apply(program,p->get_scratch1(),{{p->get_terrain(),"to_be_copied"}});
	p->remove_texture(tmp);
	p->get_terrain()->swap(p->get_scratch1());
}

void BackupFilter::restoreBackup() {
	ShaderProgram *program_backup = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(copy_img->getCode(), GL_FRAGMENT_SHADER)
			.link();
	p->apply(program_backup,target(p),{{tmp,"to_be_copied"}});
}


ProgressFilter::ProgressFilter(Project *p, std::function<Texture *(Project *)> target, SubFilter* subfilter) : BackupFilter(p, target) {
	this->subFilter = subfilter;
	progressModal = new Modal("Applying filter",[this](Project* p) {
		ImGui::ProgressBar(this->progress,ImVec2(360,0));
		bool a = ImGui::Button("Cancel");
		if (a) {
			aborting = true;
		}
		return a;
	});
	progressModal->open();
}

void ProgressFilter::progressBar(float a) {
	progress = a;
	progressModal->update(p);
}

void ProgressFilter::run() {
	if (finished) return;
	auto [f, progress] = subFilter->step();

	finished = f;

	progressBar(progress);

	if (aborting) {
		restoreBackup();
		p->finalizeFilter();
	}

	if(finished) {
		restoreUnselected();
		add_history();
		p->finalizeFilter();
	}
}

ProgressFilter::~ProgressFilter() {

}

bool ProgressFilter::isFinished() {
	return finished;
}
