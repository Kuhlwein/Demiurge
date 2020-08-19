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

	const char* items[] = { "Erode","Dilate"};
	static int current = 0;
	ImGui::Combo("Operation",&current, items,IM_ARRAYSIZE(items));
}

std::shared_ptr<BackupFilter> cpufilterMenu::makeFilter(Project *p) {
	auto morph = std::make_unique<cpufilter>(p);
	return std::make_shared<ProgressFilter>(p,[](Project* p){return p->get_terrain();},std::move(morph));
}



cpufilter::cpufilter(Project *p) {

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

	for (int i=0; i<N; i++) {

		auto flowfilter = new FlowFilter(0.5,1);
		std::pair<bool, float> progress;
		do {
			dispatchGPU([&flowfilter, &progress](Project *p) {
				progress = flowfilter->step(p);
			});
			setProgress({false,((float) i) /N+ progress.second/N});
		} while (!progress.first);



		dispatchGPU([&heightdata, &updrift](Project *p) {
			p->get_scratch2()->uploadData(GL_RED, GL_FLOAT, heightdata.get());
			p->get_terrain()->swap(p->get_scratch2());
			//Terrain is now heightdata
			//scratch2 is now flow

			Shader *shader = Shader::builder()
					.include(fragmentBase)
					.include(cornerCoords)
					.include(p->canvas->projection_shader())
					.include(get_slope)
					.create("", R"(
	fc = max(texture2D(img,st).r - 4*pow(texture2D(scratch2,st).r,0.5)*tan(get_slope(1,st)),0);
)");//TODO SLOPE SHOULD NOT BE RADIANS, BUT GRADIENT
			ShaderProgram *program = ShaderProgram::builder()
					.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
					.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
					.link();

			program->bind();
			p->setCanvasUniforms(program);
			p->apply(program, p->get_scratch1());
			//terrain is heightdata - flow
			//scratch1 is updrift

			delete shader;
			delete program;


			p->get_terrain()->uploadData(GL_RED, GL_FLOAT, updrift.get());
			p->get_terrain()->swap(p->get_scratch1());
			shader = Shader::builder()
					.include(fragmentBase)
					.include(def_pi)
					.include(cornerCoords)
					.include(get_slope)
					.create("", R"(
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
	//float width = (circumference*(cornerCoords[1]-cornerCoords[0])/(2*M_PI) / textureSize(img,0).y);
	//float slopef = max( ( 0.05 - (texture2D(img,st).r - h)/width )*width ,0);

	fc = texture2D(img,st).r + min(hdiff,texture2D(scratch1,st).r);
)");
			program = ShaderProgram::builder()
					.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
					.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
					.link();

			program->bind();
			p->setCanvasUniforms(program);
			p->apply(program, p->get_terrain());
			heightdata = p->get_terrain()->downloadDataRAW();
		});

		delete flowfilter;
	}


	setProgress({true,1.0f});
}

cpufilter::~cpufilter() {
	std::cout << "\tdestroying cpu filter\n";
}


