//
// Created by kuhlwein on 7/25/20.
//

#include "Project.h"
#include <imgui/imgui.h>
#include <algorithm>
#include "Blur.h"

Blur::Blur() : Modal("Blur", [this](Project* p) {
	return this->update_self(p);
}) {

}

bool Blur::update_self(Project *p) {
	ImGuiIO io = ImGui::GetIO();

	ImGui::DragFloat("Radius", &radius, 0.01f, 0.1f, 100.0f, "%.2f", 1.0f);

	static bool previewing = false;

	if(ImGui::Button("Preview")) {
		if (previewing) {
			p->undo();
		}
		previewing = true;
		p->NEW_dispatchFilter(std::move(std::make_unique<BlurFilter>(p,radius)));
		return false;
	}
	ImGui::SameLine();
	if(ImGui::Button("Apply")) {
		if (!previewing) p->NEW_dispatchFilter(std::move(std::make_unique<BlurFilter>(p,radius)));
		previewing = false;
		return true;
	}
	ImGui::SameLine();
	if(ImGui::Button("Close")) {
		if (previewing) p->undo();
		previewing=false;
		return true;
	}

	return false;
}




BlurFilter::BlurFilter(Project *p, float radius) : BackupFilter(p,[](Project* p){return p->get_terrain();}) {
	this->radius = radius;
}

BlurFilter::~BlurFilter() {

}

void BlurFilter::run() {
	BlurFilter::blur(p,radius,p->get_terrain());
	p->finalizeFilter();
}

void BlurFilter::finalize() {
	restoreUnselected();
	add_history();
}

//Todo geometry for blur, also progress bar
void BlurFilter::blur(Project *p, float radius, Texture *target) {
	radius = radius/2; //radius vs diameter??
	//Uses linear sampling
	auto t1 = new Texture(target->getWidth(),target->getHeight(),GL_R32F,"img",GL_LINEAR);
	auto t2 = new Texture(target->getWidth(),target->getHeight(),GL_R32F,"img",GL_LINEAR);

	ShaderProgram *copyProgram = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(copy_img->getCode(), GL_FRAGMENT_SHADER)
			.link();
	p->apply(copyProgram, t1, {{target, "to_be_copied"}});

	Shader* blurshader = Shader::builder()
			.create(R"(
float blur13(sampler2D image, vec2 uv, vec2 direction) {
  float color = 0.0f;
  vec2 resolution = textureSize(image,0);
  vec2 off1 = vec2(1.411764705882353) * direction;
float phi = (uv.y-0.5)*3.14159;
off1.x = off1.x/cos(abs(phi));
  vec2 off2 = vec2(3.2941176470588234) * direction;
off2.x = off2.x/cos(abs(phi));
  vec2 off3 = vec2(5.176470588235294) * direction;
off3.x = off3.x/cos(abs(phi));
  color += texture2D(image, uv).r * 0.1964825501511404;
  color += texture2D(image, offset(uv, off1,resolution)).r * 0.2969069646728344;
  color += texture2D(image, offset(uv,-off1,resolution)).r * 0.2969069646728344;
  color += texture2D(image, offset(uv, off2,resolution)).r * 0.09447039785044732;
  color += texture2D(image, offset(uv,-off2,resolution)).r * 0.09447039785044732;
  color += texture2D(image, offset(uv, off3,resolution)).r * 0.010381362401148057;
  color += texture2D(image, offset(uv,-off3,resolution)).r * 0.010381362401148057;
  return color;
}
)");

	Shader* fragment_set = Shader::builder()
			.include(fragmentBase)
			.include(p->getGeometry()->offset())
			.include(blurshader)
			.create("uniform vec2 direction;",R"(
fc = blur13(img,st,direction);
)");
	ShaderProgram *blurProgram = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(fragment_set->getCode(), GL_FRAGMENT_SHADER)
			.link();

	int id = glGetUniformLocation(blurProgram->getId(), "direction");

	//todo: needs work on consistensy at  low R
	float R = radius*radius/2;
	//std::cout << "R " << R << "\n";
	std::vector<float> rlist = {};
	float i = 1.0f;
	float incrementer = 0.5f;
	if (R<3) {
		float k = 1/sqrt(55/R);
		incrementer = k;
		i= k;
	}
	while (R>=i*i) {
		R-=i*i;
		rlist.push_back(i);
		i+=incrementer;
	}
	if (R>0.0f) rlist.push_back(sqrt(R));
	std::sort(rlist.begin(),rlist.end());

	//std::cout << "begin: ";
	//for (auto a : rlist) std::cout << a << " hej ";

	for (float a : rlist) {
		glUniform2f(id,0,a);
		p->apply(blurProgram, t2, {{t1, "img"}});
		glUniform2f(id,a,0);
		p->apply(blurProgram, t1, {{t2, "img"}});
	}

	p->apply(copyProgram, target, {{t1, "to_be_copied"}});

	delete t1;
	delete t2;
	delete copyProgram;
	delete blurProgram;
	delete fragment_set;
	delete blurshader;
}
