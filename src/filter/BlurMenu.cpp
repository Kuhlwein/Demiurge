//
// Created by kuhlwein on 7/25/20.
//

#include "Project.h"
#include <imgui/imgui.h>
#include <algorithm>
#include "BlurMenu.h"

BlurMenu::BlurMenu() : FilterModal("Blur") {

}

void BlurMenu::update_self(Project *p) {
	ImGuiIO io = ImGui::GetIO();

	ImGui::DragFloat("Radius", &radius, 0.01f, 0.1f, 100.0f, "%.2f", 1.0f);
}

std::shared_ptr<BackupFilter> BlurMenu::makeFilter(Project* p) {
	return std::make_shared<ProgressFilter>(p, [](Project* p){return p->get_terrain();},std::move(std::make_unique<Blur>(p, radius, p->get_terrain())));
}

Blur::Blur(Project *p, float radius, Texture *target) : SubFilter() {
	this->target = target;

	radius = radius/2; //radius vs diameter??
	//Uses linear sampling
	tex1 = new Texture(target->getWidth(),target->getHeight(),GL_R32F,"img",GL_LINEAR);
	tex2 = new Texture(target->getWidth(),target->getHeight(),GL_R32F,"img",GL_LINEAR);

	copyProgram = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(copy_img->getCode(), GL_FRAGMENT_SHADER)
			.link();
	p->apply(copyProgram, tex1, {{target, "to_be_copied"}});

	Shader* blurshader = Shader::builder()
			.include(cornerCoords)
			.create(R"(
float blur13(sampler2D image, vec2 uv, vec2 direction) {
  	float color = 0.0f;
	vec2 resolution = textureSize(image,0);

  	vec2 off1 = vec2(1.411764705882353) * direction;
  	vec2 off2 = vec2(3.2941176470588234) * direction;
  	vec2 off3 = vec2(5.176470588235294) * direction;

	float phifactor = cos(abs(tex_to_spheric(uv).y));
	off1.x = off1.x/phifactor;
	off2.x = off2.x/phifactor;
	off3.x = off3.x/phifactor;

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
			.include(cornerCoords)
			.include(blurshader)
			.create("uniform vec2 direction;",R"(
fc = blur13(img,st,direction);
)");
	blurProgram = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(fragment_set->getCode(), GL_FRAGMENT_SHADER)
			.link();


	//todo: needs work on consistensy at  low R
	float R = radius*radius/2;
	rlist = {};
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
}

std::pair<bool,float> Blur::step(Project* p) {
	blurProgram->bind();
	int id = glGetUniformLocation(blurProgram->getId(), "direction");
	glUniform2f(id,0,rlist[i]);
	p->setCanvasUniforms(blurProgram);
	p->apply(blurProgram, tex2, {{tex1, "img"}});
	glUniform2f(id,rlist[i],0);
	p->apply(blurProgram, tex1, {{tex2, "img"}});


	i++;

	bool finished = false;

	if(i>rlist.size()-1) {
		p->apply(copyProgram, target, {{tex1, "to_be_copied"}});
		finished = true;
	}

	return {finished,(float)(i)/(rlist.size())};
}
