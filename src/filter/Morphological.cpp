//
// Created by kuhlwein on 7/31/20.
//

#include "Project.h"
#include <imgui/imgui.h>
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
	this->radius = radius;
	this->target = target;

	tex = new Texture(target->getWidth(),target->getHeight(),GL_R32F,"img",GL_NEAREST);

	Shader* defineErode = Shader::builder()
			.include(p->getGeometry()->offset())
			.create(R"(
float erode(sampler2D image, vec2 uv, float radius) {
	vec2 resolution = textureSize(image,0);
	float a = texture(image,uv).r;

float phi = (uv.y-0.5)*3.14159;
float factor = 1/cos(abs(phi));

int N = 50;
for (int i=0; i<N; i++) {
	a = max(a,texture(image, offset(uv, vec2(cos(2*3.14159*i/N)*radius*factor,sin(2*3.14159*i/N)*radius),resolution)).r);
}

//a = max(a,texture(image, offset(uv, vec2(radius*factor,0),resolution)).r);
//a = max(a,texture(image, offset(uv, vec2(-radius*factor,0),resolution)).r);
//a = max(a,texture(image, offset(uv, vec2(0,radius),resolution)).r);
//a = max(a,texture(image, offset(uv, vec2(0,-radius),resolution)).r);

//a = max(a,texture(image, offset(uv, vec2(radius,radius),resolution)).r);
//a = max(a,texture(image, offset(uv, vec2(-radius,-radius),resolution)).r);
//a = max(a,texture(image, offset(uv, vec2(-radius,radius),resolution)).r);
//a = max(a,texture(image, offset(uv, vec2(radius,-radius),resolution)).r);



//int r = 5;
//for (int i=-r; i<=r; i++) for (int j=-r; j<=r; j++) {
//if(i*i+j*j<=(r)*(r+0.5)) a = max(a,texture(image, offset(uv, vec2(i*factor,j),resolution)).r);
//if(i*i+j*j<=(r)*(r+0.5)) a = max(a,texture(image, offset(uv, vec2(i,j),resolution)).r);
//}

	return a;
}
)","");

	Shader* fragment_set = Shader::builder()
			.include(fragmentBase)
			.include(defineErode)
			.create("uniform float radius; uniform sampler2D tex;",R"(
fc = erode(tex,st,radius);
)");
	erodeProgram = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(fragment_set->getCode(), GL_FRAGMENT_SHADER)
			.link();

	copyProgram = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(copy_img->getCode(), GL_FRAGMENT_SHADER)
			.link();
}

std::pair<bool, float> Erode::step() {
	copyProgram->bind();
	p->apply(copyProgram,tex,{{p->get_terrain(), "to_be_copied"}});

	erodeProgram->bind();
	int id = glGetUniformLocation(erodeProgram->getId(), "radius");
	glUniform1f(id,steps+1); //TODO factor on steps
	p->apply(erodeProgram, p->get_terrain(),{{tex, "tex"}});
	steps++;
	return {steps>=radius,(float(steps))/radius};
}

ErodeTerrain::ErodeTerrain(Project *p, float radius) : ProgressFilter(p, [](Project* p){return p->get_terrain();}) {
	erode = new Erode(p,radius,p->get_terrain());
}

ErodeTerrain::~ErodeTerrain() {

}

std::pair<bool, float> ErodeTerrain::step() {
	return erode->step();
}
