//
// Created by kuhlwein on 7/31/20.
//

#include "Project.h"
#include <imgui/imgui.h>
#include <iostream>
#include <algorithm>
#include "Morphological.h"

MorphologicalMenu::MorphologicalMenu() : FilterModal("Erode/Dilate") {

}

void MorphologicalMenu::update_self(Project *p) {
	ImGuiIO io = ImGui::GetIO();

	const char* items[] = { "Erode","Dilate"};
	ImGui::Combo("Operation",&current, items,IM_ARRAYSIZE(items));
	ImGui::DragFloat("Radius", &radius, 0.01f, 0.1f, 100.0f, "%.2f", 1.0f);
}

std::shared_ptr<BackupFilter> MorphologicalMenu::makeFilter(Project *p) {
	auto morph = std::make_unique<Morphological>(p, radius, p->get_terrain(),(current==0) ? "min" : "max");
	return std::make_shared<ProgressFilter>(p,[](Project* p){return p->get_terrain();},std::move(morph));
}

Morphological::Morphological(Project *p, float radius, Texture *target, std::string operation) : SubFilter() {
	this->target = target;

	r = {};
	int x = 1;
	while (radius>=0) {
		if (x<radius) {
			radius-=x;
			r.push_back(x);
			x*=2;
		} else {
			r.push_back(radius);
			break;
		}
	}
	std::sort(r.begin(),r.end());

	Shader* defineErode = Shader::builder()
			.include(offset_shader)
			.include(cornerCoords)
			.create(R"(
float erode(sampler2D image, vec2 uv, float radius) {
	vec2 resolution = textureSize(image,0);
	float a = texture(image,uv).r;

float phi = tex_to_spheric(uv).y;
float factor = 1/cos(abs(phi));

int N = 64;
for (int i=0; i<N; i++) {
	a = )"+operation+R"((a,texture(image, offset(uv, vec2(cos(2*3.14159*i/N)*radius*factor,sin(2*3.14159*i/N)*radius),resolution)).r);
}
	return a;
}
)","");

	Shader* fragment_set = Shader::builder()
			.include(fragmentBase)
			.include(defineErode)
			.create("uniform float radius; uniform sampler2D target;",R"(
fc = erode(target,st,radius);
)");
	erodeProgram = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(fragment_set->getCode(), GL_FRAGMENT_SHADER)
			.link();
}

std::pair<bool, float> Morphological::step(Project* p) {
	erodeProgram->bind();
	int id = glGetUniformLocation(erodeProgram->getId(), "radius");
	glUniform1f(id,r[steps]);
	p->setCanvasUniforms(erodeProgram);
	p->apply(erodeProgram, p->get_scratch1(),{{target,"target"}});
	p->get_scratch1()->swap(target);
	steps++;

	return {steps>=r.size(),(float(steps))/r.size()};
}

MorphologicalGradient::MorphologicalGradient(Project *p, float radius, Texture *target) {
	dilate = new Morphological(p,(radius+1)/2,target,"max");
	erode = new Morphological(p,radius/2,target,"min");
	this->target = target;
	tmp = new Texture(target->getWidth(),target->getHeight(),GL_R32F,"img",GL_NEAREST);

	auto copyProgram = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(copy_img->getCode(), GL_FRAGMENT_SHADER)
			.link();
	p->apply(copyProgram, tmp, {{target, "to_be_copied"}});

}

std::pair<bool, float> MorphologicalGradient::step(Project *p) {
	float progress;
	if (!finish_part1) {
		auto [finished,f] = erode->step(p);
		finish_part1 = finished;
		progress = f*0.5f;
		if (finished) tmp->swap(target);
	} else {
		auto [finished,f] = dilate->step(p);
		progress = 0.5f + f*0.5f;
		if (finished) {
			auto difference = Shader::builder()
					.include(fragmentBase)
					.create("uniform sampler2D eroded; uniform sampler2D dilated;",R"(
fc = texture(dilated, st).r - texture(eroded, st).r;
)");

			auto copyProgram = ShaderProgram::builder()
					.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
					.addShader(difference->getCode(), GL_FRAGMENT_SHADER)
					.link();
			p->apply(copyProgram, p->get_scratch1(), {{target, "dilated"},{tmp,"eroded"}});
			p->get_scratch1()->swap(target);

			return {true,1.0f};
		}
	}


	return {false, progress};
}
