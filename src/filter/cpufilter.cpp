//
// Created by kuhlwein on 8/10/20.
//

#include "Project.h"
#include <imgui/imgui.h>
#include <iostream>
#include <thread>
#include "cpufilter.h"
#include "FlowFilter.h"
#include "ThermalErosion.h"


cpufilterMenu::cpufilterMenu() : FilterModal("cpufilter") {
	std::cout << "\tcreating cpufilter\n";
}

void cpufilterMenu::update_self(Project *p) {
	ImGuiIO io = ImGui::GetIO();

	ImGui::DragFloat("Exponent", &exponent, 0.01f, 0.0f, 100.0f, "%.2f", 1.0f);
	ImGui::DragFloat("slope exponent", &sexponent, 0.01f, 0.0f, 100.0f, "%.2f", 1.0f);
	ImGui::DragFloat("factor", &factor, 0.01f, 0.0f, 100.0f, "%.2f", 1.0f);
	ImGui::DragInt("toggle",&dolakes);
}

std::shared_ptr<BackupFilter> cpufilterMenu::makeFilter(Project *p) {
	auto morph = std::make_unique<cpufilter>(p,exponent, sexponent, factor,dolakes);
	return std::make_shared<ProgressFilter>(p,[](Project* p){return p->get_terrain();},std::move(morph));
}



cpufilter::cpufilter(Project *p, float exponent, float slope_exponent, float factor, int dolakes) {
	this->exponent = exponent;
	this->sexponent = slope_exponent;
	this->factor = factor;
	this->dolakes = dolakes;
}

void cpufilter::run() {
	int N = 50;

	std::unique_ptr<float[]> heightdata;
	std::unique_ptr<float[]> updrift;

	dispatchGPU([&heightdata,&updrift,&N](Project* p){
		Shader* shader = Shader::builder()
				.include(fragmentBase)
				.include(cornerCoords)
				.create("",R"(
	fc = max(texture2D(img,st).r,0)/)"+std::to_string(N)+R"(;
)");
		ShaderProgram* program = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
				.link();

		program->bind();
		p->setCanvasUniforms(program);
		p->apply(program,p->get_scratch1());
		delete shader;
		delete program;
		updrift = p->get_scratch1()->downloadDataRAW();

		shader = Shader::builder()
				.include(fragmentBase)
				.include(cornerCoords)
				.create("",R"(
	float val = texture2D(img,st).r;
	if (val<=0) fc=val;
	else fc = val/)"+std::to_string(N)+R"(;
)");
		program = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
				.link();

		program->bind();
		p->setCanvasUniforms(program);
		p->apply(program,p->get_scratch1());
		delete shader;
		delete program;
		heightdata = p->get_scratch1()->downloadDataRAW();
	});

	Texture* updrifttex;
	dispatchGPU([&updrifttex, &updrift](Project *p) {
		updrifttex = new Texture(p->getWidth(),p->getHeight(),GL_R32F,"updrift",GL_NEAREST);
		updrifttex->uploadData(GL_RED, GL_FLOAT, updrift.get());
	});

	for (int i=0; i<N; i++) {

		auto flowfilter = new FlowFilter(0.5,exponent,dolakes>0);
		std::pair<bool, float> progress;
		do {
			dispatchGPU([&flowfilter, &progress](Project *p) {
				progress = flowfilter->step(p);
			});
			setProgress({false,((float) i) /N+ progress.second/N});
		} while (!progress.first);



		dispatchGPU([this,&heightdata, &updrift, &updrifttex](Project *p) {
			p->get_scratch2()->uploadData(GL_RED, GL_FLOAT, heightdata.get());
			p->get_terrain()->swap(p->get_scratch2());
			//Terrain is now heightdata
			//scratch2 is now flow

			Shader *shader = Shader::builder()
					.include(fragmentBase)
					.include(cornerCoords)
					.include(p->canvas->projection_shader())
					.include(get_slope)
					.create("uniform float factor; uniform float slope_exponent;", R"(
	fc = max(texture2D(img,st).r - factor*4*pow(texture2D(scratch2,st).r,0.5)* pow(tan(get_slope(1,st)),slope_exponent) ,0);
)");//TODO SLOPE SHOULD NOT BE RADIANS, BUT GRADIENT
			ShaderProgram *program = ShaderProgram::builder()
					.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
					.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
					.link();

			//program->bind();
			//p->setCanvasUniforms(program);
			//p->apply(program, p->get_scratch1());
			//terrain is heightdata - flow
			//scratch1 is updrift

			delete shader;
			delete program;


			//p->get_terrain()->swap(p->get_scratch1());
			shader = Shader::builder()
					.include(fragmentBase)
					.include(def_pi)
					.include(cornerCoords)
					.include(get_slope)
					.create(R"(
	uniform sampler2D updrift;
	uniform float factor;
	uniform float slope_exponent;
)", R"(
	float h = texture2D(img,st).r;
	float h2;

	vec2 psize = pixelsize(st);
	float maxslope=0;
	float dist = length(psize);
	float ndist;

	vec2 resolution = textureSize(img,0);


	h2 = texture2D(img, offset(st, vec2(1,1),resolution)).r;
	ndist = length(psize*vec2(1,1));
	h2 = (h-h2)/ndist;
	if (h2>maxslope) {
		maxslope = h2;
		dist = ndist;
	}
	h2 = texture2D(img, offset(st, vec2(0,1),resolution)).r;
	ndist = length(psize*vec2(0,1));
	h2 = (h-h2)/ndist;
	if (h2>maxslope) {
		maxslope = h2;
		dist = ndist;
	}
	h2 = texture2D(img, offset(st, vec2(-1,1),resolution)).r;
	ndist = length(psize*vec2(-1,1));
	h2 = (h-h2)/ndist;
	if (h2>maxslope) {
		maxslope = h2;
		dist = ndist;
	}
	h2 = texture2D(img, offset(st, vec2(1,0),resolution)).r;
	ndist = length(psize*vec2(1,0));
	h2 = (h-h2)/ndist;
	if (h2>maxslope) {
		maxslope = h2;
		dist = ndist;
	}
	h2 = texture2D(img, offset(st, vec2(-1,0),resolution)).r;
	ndist = length(psize*vec2(-1,0));
	h2 = (h-h2)/ndist;
	if (h2>maxslope) {
		maxslope = h2;
		dist = ndist;
	}
	h2 = texture2D(img, offset(st, vec2(1,-1),resolution)).r;
	ndist = length(psize*vec2(1,-1));
	h2 = (h-h2)/ndist;
	if (h2>maxslope) {
		maxslope = h2;
		dist = ndist;
	}
	h2 = texture2D(img, offset(st, vec2(0,-1),resolution)).r;
	ndist = length(psize*vec2(0,-1));
	h2 = (h-h2)/ndist;
	if (h2>maxslope) {
		maxslope = h2;
		dist = ndist;
	}
	h2 = texture2D(img, offset(st, vec2(-1,-1),resolution)).r;
	ndist = length(psize*vec2(-1,-1));
	h2 = (h-h2)/ndist;
	if (h2>maxslope) {
		maxslope = h2;
		dist = ndist;
	}

	float SLOPE = 0.05;
	float hdiff = SLOPE*dist-maxslope*dist;


	float flow = factor*4*pow(texture2D(scratch2,st).r,0.5)* pow(tan(get_slope(1,st)),slope_exponent);
	float up = texture2D(updrift,st).r;
	float terrain = texture2D(img,st).r;
	fc = terrain + min(hdiff, max(0,up-flow) );
)");
			program = ShaderProgram::builder()
					.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
					.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
					.link();
			p->add_texture(updrifttex);
			program->bind();

			int id = glGetUniformLocation(program->getId(),"slope_exponent");
			glUniform1f(id, sexponent);
			id = glGetUniformLocation(program->getId(),"factor");
			glUniform1f(id, factor);

			p->setCanvasUniforms(program);
			p->apply(program, p->get_terrain());
			heightdata = p->get_terrain()->downloadDataRAW();
			p->remove_texture(updrifttex);
		});

		delete flowfilter;
	}

	setProgress({true,1.0f});
}

cpufilter::~cpufilter() {
	std::cout << "\tdestroying cpu filter\n";
}


