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
	int N = 250;

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

		auto flowfilter = new FlowFilter(0.5);
		std::pair<bool, float> progress;
		do {
			dispatchGPU([&flowfilter, &progress](Project *p) {
				progress = flowfilter->step(p);
			});
			setProgress({false,((float) i) /N+ progress.second/N});
		} while (!progress.first);

		dispatchGPU([&heightdata, &updrift](Project *p) {
			p->get_scratch1()->uploadData(GL_RED, GL_FLOAT, updrift.get());

			Shader *shader = Shader::builder()
					.include(fragmentBase)
					.include(cornerCoords)
					.include(p->canvas->projection_shader())
					.include(get_slope)
					.create("", R"(
	fc = max(texture2D(scratch1,st).r - 1/250*pow(texture2D(img,st).r,0.5)*tan(get_slope(1,st)),0);
)");//TODO SLOPE SHOULD NOT BE RADIANS, BUT GRADIENT
			ShaderProgram *program = ShaderProgram::builder()
					.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
					.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
					.link();

			program->bind();
			p->setCanvasUniforms(program);
			p->apply(program, p->get_scratch2());

			delete shader;
			delete program;

			p->get_scratch1()->uploadData(GL_RED, GL_FLOAT, heightdata.get());
			shader = Shader::builder()
					.include(fragmentBase)
					.include(cornerCoords)
					.include(get_slope)
					.create("", R"(
	float slope = get_slope(1,st);
	if (slope>M_PI/6/100) {
		fc = texture2D(scratch2,st).r;
	} else {
		fc = texture2D(scratch1,st).r + texture2D(scratch2,st).r;
	}
	//fc = texture2D(scratch1,st).r + texture2D(scratch2,st).r;
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


//		auto thermal = new ThermalErosion();
//		do {
//			dispatchGPU([&thermal, &progress](Project *p) {
//				progress = thermal->step(p);
//			});
//		} while (!progress.first);




		delete flowfilter;
	}


	setProgress({true,1.0f});
}

cpufilter::~cpufilter() {
	std::cout << "\tdestroying cpu filter\n";
}


