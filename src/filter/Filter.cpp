//
// Created by kuhlwein on 7/18/20.
//

#include "Project.h"
#include <Shader.h>
#include <iostream>
#include <thread>
#include "Filter.h"
#include "Texture.h"

BackupFilter::BackupFilter(Project *p, std::function<Texture *(Project *p)> target) : Filter() {
	this->target = target;
	this->p = p;
	std::cout << "creating backup\n";
	tmp = new Texture(p->getWidth(),p->getHeight(),GL_R32F,"tmp");

	ShaderProgram *program_backup = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(copy_img->getCode(), GL_FRAGMENT_SHADER)
			.link();
	p->apply(program_backup,tmp,{{target(p),"to_be_copied"}});
}

BackupFilter::~BackupFilter() {
	std::cout << "\tdestroy backupfilter\n";
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


ProgressFilter::ProgressFilter(Project *p, std::function<Texture *(Project *)> target, std::unique_ptr<SubFilter> subfilter) : BackupFilter(p, target) {
	this->subFilter = std::move(subfilter);
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

void ProgressFilter::run(Project* p) {
	if (finished) return;
	auto [f, progress] = subFilter->step(p);

	finished = f;

	progressBar(progress);

	if (aborting) {
		restoreBackup();
		p->finalizeFilter();
	}

	if(finished) {
		//restoreUnselected(); //TODO move elsewhere
		//add_history();
		p->finalizeFilter();
	}
}

ProgressFilter::~ProgressFilter() {
	std::cout << "\tdestroying progressfilter\n";
}

bool ProgressFilter::isFinished() {
	return finished;
}

std::pair<bool, float> AsyncSubFilter::step(Project *p) {
	if (first) {
		progress = {false,0.0f};
		t = std::thread([this]{this->run();});
		first = false;
	}
	std::unique_lock<std::mutex> lk(gpu_mtx);
	if (runningGPU) {
		f(p);
		runningGPU = false;
		cv.notify_all();
	}

	auto progress = getProgress();
	if (progress.first) {
		t.join();
	}
	return progress;
}

std::pair<bool, float> AsyncSubFilter::getProgress() {
	std::unique_lock<std::mutex> lk(progress_mtx);
	std::pair<bool, float> p;
	p = progress;
	return p;
}

void AsyncSubFilter::setProgress(std::pair<bool, float> p) {
	std::unique_lock<std::mutex> lk(progress_mtx);
	progress = p;
}

AsyncSubFilter::AsyncSubFilter() {

}

void AsyncSubFilter::dispatchGPU(std::function<void(Project *)> f) {
	std::unique_lock<std::mutex> lk(gpu_mtx);
	this->f = f;
	runningGPU = true;
	cv.wait(lk); //wait for function to finish
}



Shader *filter::blendMode() {
	ImGuiIO io = ImGui::GetIO();

	static Shader* replace = Shader::builder().create(R"(
float blend_mode(float old, float new, float selection) {
	return old*(1-selection) + new*selection;
}
)","");
	static Shader* add = Shader::builder().create(R"(
float blend_mode(float old, float new, float selection) {
	return old + selection*new;
}
)","");
	static Shader* subtract = Shader::builder().create(R"(
float blend_mode(float old, float new, float selection) {
	return max(old-new,0);
}
)","");
	static Shader* multiply = Shader::builder().create(R"(
float blend_mode(float old, float new, float selection) {
	return old*new;
}
)","");
	static Shader* divide = Shader::builder().create(R"(
float blend_mode(float old, float new, float selection) {
	return old/new;
}
)","");
	static Shader* max = Shader::builder().create(R"(
float blend_mode(float old, float new, float selection) {
	return max(old,new);
}
)","");
	static Shader* min = Shader::builder().create(R"(
float blend_mode(float old, float new, float selection) {
	return min(old,new);
}
)","");

	static int current = 0;
	static bool pressed = false;

	if (io.KeyShift || io.KeyCtrl) pressed = true;
	if (io.KeyShift && io.KeyCtrl) current = 3;
	else if (io.KeyShift) current = 1;
	else if (io.KeyCtrl) current = 2;
	else if (pressed) {
		current = 0;
		pressed = false;
	}

	const char* items[] = { "Replace","Add", "Subtract","Multiply","Divide","Max","Min"};
	ImGui::Combo("Blend mode",&current,items,IM_ARRAYSIZE(items));
	switch (current) {
		case 0:
			return replace;
		case 1:
			return add;
		case 2:
			return subtract;
		case 3:
			return multiply;
		case 4:
			return divide;
		case 5:
			return max;
		case 6:
			return min;
	}
}
