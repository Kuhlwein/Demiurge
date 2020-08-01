//
// Created by kuhlwein on 7/31/20.
//

#include "Project.h"
#include <imgui/imgui.h>
#include <iostream>
#include <algorithm>
#include "Morphological.h"

ErodeMenu::ErodeMenu() : FilterModal("Erode") {

}

void ErodeMenu::update_self(Project *p) {
	ImGuiIO io = ImGui::GetIO();

	ImGui::DragFloat("Radius", &radius, 0.01f, 0.1f, 100.0f, "%.2f", 1.0f);
}

std::shared_ptr<Filter> ErodeMenu::makeFilter(Project *p) {
	return std::make_shared<ErodeTerrain>(p,radius);
}

Erode::Erode(Project *p, float radius, Texture *target) : SubFilter(p) {
	this->target = target;

	r = {};
	int x = 1;
	while (radius>=0) {
		if (x<radius) {
			radius-=x;
			r.push_back(x);
			std::cout << x << "\n";
			x*=2;
		} else {
			r.push_back(radius);
			std::cout << radius << "\n";
			break;
		}
	}
	std::sort(r.begin(),r.end());

	Shader* defineErode = Shader::builder()
			.include(p->getGeometry()->offset())
			.create(R"(
float erode(sampler2D image, vec2 uv, float radius) {
	vec2 resolution = textureSize(image,0);
	float a = texture(image,uv).r;

float phi = (uv.y-0.5)*3.14159;
float factor = 1/cos(abs(phi));

int N = 64;
for (int i=0; i<N; i++) {
	a = max(a,texture(image, offset(uv, vec2(cos(2*3.14159*i/N)*radius*factor,sin(2*3.14159*i/N)*radius),resolution)).r);
}
	return a;
}
)","");

	Shader* fragment_set = Shader::builder()
			.include(fragmentBase)
			.include(defineErode)
			.create("uniform float radius;",R"(
fc = erode(img,st,radius);
)");
	erodeProgram = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(fragment_set->getCode(), GL_FRAGMENT_SHADER)
			.link();
}

std::pair<bool, float> Erode::step() {
	erodeProgram->bind();
	int id = glGetUniformLocation(erodeProgram->getId(), "radius");
	glUniform1f(id,r[steps]); //TODO factor on steps
	p->apply(erodeProgram, p->get_scratch1());
	p->get_scratch1()->swap(p->get_terrain());
	steps++;

	return {steps>=r.size(),(float(steps))/r.size()};
}

ErodeTerrain::ErodeTerrain(Project *p, float radius) : ProgressFilter(p, [](Project* p){return p->get_terrain();}) {
	erode = new Erode(p,radius,p->get_terrain());
}

ErodeTerrain::~ErodeTerrain() {

}

std::pair<bool, float> ErodeTerrain::step() {
	return erode->step();
}
