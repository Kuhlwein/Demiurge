//
// Created by kuhlwein on 8/12/20.
//

#include "Project.h"
#include <imgui/imgui.h>
#include <iostream>
#include "FlowFilter.h"

FlowfilterMenu::FlowfilterMenu() : FilterModal("Flowfilter") {

}

void FlowfilterMenu::update_self(Project *p) {
	ImGuiIO io = ImGui::GetIO();

	const char* items[] = { "Erode","Dilate"};
	static int current = 0;
	ImGui::Combo("Operation",&current, items,IM_ARRAYSIZE(items));
}

std::shared_ptr<BackupFilter> FlowfilterMenu::makeFilter(Project *p) {
	auto morph = std::make_unique<FlowFilter>();
	return std::make_shared<ProgressFilter>(p,[](Project* p){return p->get_terrain();},std::move(morph));
}

FlowFilter::FlowFilter() {

}

FlowFilter::~FlowFilter() {

}

bool Nthbit(int num, int N) {
	return num & (1 << (N-1));
}

bool isLake(int) {
	return true;
}

void FlowFilter::run() {
	/*
	 * all bits zero -> not point of interest
	 *
	 * Bits set for neighbour, 5'th bit is self and indicates a sink/lake
	 * 1 2 3
	 * 4 5 6
	 * 7 8 9
	 *
	 * the 10'th bit indicates that this is a border sink/lake, aka a river mouth
	 */

	//Find magic numbers
	dispatchGPU([this](Project* p){
		Shader* shader = Shader::builder()
				.include(fragmentBase)
				.include(offset_shader)
				.create("",R"(
	vec2 resolution = textureSize(img,0);

	float a = texture2D(img, st).r;
	fc = 0.0;
	if (a<=0.0f) return;
	if (texture2D(sel, st).r==0) return;
	fc = 5.0;
	float a2;

	a2 = texture2D(img, offset(st, vec2(1,1),resolution)).r;
	if(a2<a) {
		fc = 1;
		a = a2;
	}
	a2 = texture2D(img, offset(st, vec2(0,1),resolution)).r;
	if(a2<a) {
		fc = 2;
		a = a2;
	}
	a2 = texture2D(img, offset(st, vec2(-1,1),resolution)).r;
	if(a2<a) {
		fc = 3;
		a = a2;
	}
	a2 = texture2D(img, offset(st, vec2(1,0),resolution)).r;
	if(a2<a) {
		fc = 4;
		a = a2;
	}
	a2 = texture2D(img, offset(st, vec2(-1,0),resolution)).r;
	if(a2<a) {
		fc = 6;
		a = a2;
	}
	a2= texture2D(img, offset(st, vec2(1,-1),resolution)).r;
	if(a2<a) {
		fc = 7;
		a = a2;
	}
	a2 = texture2D(img, offset(st, vec2(0,-1),resolution)).r;
	if(a2<a) {
		fc = 8;
		a = a2;
	}
	a2 = texture2D(img, offset(st, vec2(-1,-1),resolution)).r;
	if(a2<a) {
		fc = 9;
		a = a2;
	}
)");
		ShaderProgram* program = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
				.link();

		p->apply(program,p->get_scratch1());
		//p->get_scratch1()->swap(p->get_terrain());
		delete shader;
		delete program;

		shader = Shader::builder()
				.include(fragmentBase)
				.include(offset_shader)
				.create("",R"(
	vec2 resolution = textureSize(img,0);

	float a = texture2D(img, st).r;
	fc = 0.0;
	if (a<=0.0f) return;
	if (texture2D(sel, st).r==0) return;
	fc = 0.5;

	bool flag = false;

	if(texture2D(scratch1, offset(st, vec2(1,1),resolution)).r==9) fc+=1;
	if(texture2D(scratch1, offset(st, vec2(0,1),resolution)).r==8) fc+=2;
	if(texture2D(scratch1, offset(st, vec2(-1,1),resolution)).r==7) fc+=4;
	if(texture2D(scratch1, offset(st, vec2(1,0),resolution)).r==6) fc+=8;
	if(texture2D(scratch1, st).r==5) fc+=16;
	if(texture2D(scratch1, offset(st, vec2(-1,0),resolution)).r==4) fc+=32;
	if(texture2D(scratch1, offset(st, vec2(1,-1),resolution)).r==3) fc+=64;
	if(texture2D(scratch1, offset(st, vec2(0,-1),resolution)).r==2) fc+=128;
	if(texture2D(scratch1, offset(st, vec2(-1,-1),resolution)).r==1) fc+=256;

	if(texture2D(scratch1, offset(st, vec2(1,1),resolution)).r==0) flag=true;
	if(texture2D(scratch1, offset(st, vec2(0,1),resolution)).r==0) flag=true;
	if(texture2D(scratch1, offset(st, vec2(-1,1),resolution)).r==0) flag=true;
	if(texture2D(scratch1, offset(st, vec2(1,0),resolution)).r==0) flag=true;
	if(texture2D(scratch1, offset(st, vec2(-1,0),resolution)).r==0) flag=true;
	if(texture2D(scratch1, offset(st, vec2(1,-1),resolution)).r==0) flag=true;
	if(texture2D(scratch1, offset(st, vec2(0,-1),resolution)).r==0) flag=true;
	if(texture2D(scratch1, offset(st, vec2(-1,-1),resolution)).r==0) flag=true;
	if (flag) fc +=512;
)"); //TODO less samples
		program = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
				.link();
		p->apply(program,p->get_scratch2());
		p->get_scratch2()->swap(p->get_terrain());

		width = p->getWidth();
		height = p->getHeight();
	});

	struct pointdata {
		int neighbours;
		int lake;
	};

	setProgress({true,1.0f});

}
