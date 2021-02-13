//
// Created by kuhlwein on 9/21/20.
//

#include <algorithm>
#include "Project.h"
#include "DeTerrace.h"
#include "BlurMenu.h"

DeTerraceMenu::DeTerraceMenu() : FilterModal("Deterrace") {

}

void DeTerraceMenu::update_self(Project *p) {

}

std::shared_ptr<BackupFilter> DeTerraceMenu::makeFilter(Project *p) {
	return std::make_shared<ProgressFilter>(p, [](Project* p){return p->get_terrain();}, std::move(std::make_unique<DeTerrace>(p, p->get_terrain())));
}

DeTerrace::DeTerrace(Project *p, Texture *target) {
	this->p = p;
	this->target = target;

	Shader* shader = Shader::builder()
			.include(fragmentBase)
			.include(cornerCoords)
			.include(distance)
			.create(R"(
	void process(float self, vec2 dp, inout float fc) {
		vec2 resolution = textureSize(img,0);
		float epsilon = 1e-6;
		vec2 n = offset(st,dp,resolution);
		if (abs(self-texture2D(img,n).r)>epsilon) fc = min(fc,geodistance(st,n,resolution));
	}
)",R"(
	vec2 resolution = textureSize(img,0);
	fc = 1e9;
	float epsilon = 1e-6;

	float self = texture2D(img,st).r;
	process(self,vec2(1,0),fc);
	process(self,vec2(-1,0),fc);
	process(self,vec2(0,1),fc);
	process(self,vec2(0,-1),fc);
)");
	program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();

	program->bind();
	p->setCanvasUniforms(program);
	p->apply(program,p->get_scratch1());


	shader = Shader::builder()
			.include(fragmentBase)
			.include(cornerCoords)
			.include(distance)
			.create(R"(
	void process(float self, vec2 dp, inout float fc, inout float d) {
		vec2 resolution = textureSize(img,0);
		float epsilon = 1e-6;
		vec2 n = offset(st,dp,resolution);
		float nd = geodistance(st,n,resolution);
		if (abs(self-texture2D(img,n).r)>epsilon && nd<d) {
			fc = texture2D(img,n).r;
			d = nd;
		}
	}
)",R"(
	vec2 resolution = textureSize(img,0);
	fc = 0;
	float d = 1e9;
	float epsilon = 1e-6;

	float self = texture2D(img,st).r;
	process(self,vec2(1,0),fc,d);
	process(self,vec2(-1,0),fc,d);
	process(self,vec2(0,1),fc,d);
	process(self,vec2(0,-1),fc,d);
)");
	program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();

	program->bind();
	p->setCanvasUniforms(program);
	p->apply(program,p->get_scratch2());



	p->get_terrain()->swap(p->get_scratch1());





	float radius = std::min(p->getWidth(),p->getHeight());
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

	Shader* fragment_set = Shader::builder()
			.include(fragmentBase)
			.include(cornerCoords)
			.include(distance)
			.create("uniform float radius; uniform sampler2D target;",R"(
	vec2 resolution = textureSize(img,0);
	float a = texture(img,st).r;

	int N = 64;
	for (int i=0; i<N; i++) {
		vec2 c = offset(st, round(vec2(cos(2*3.14159*i/N)*radius,sin(2*3.14159*i/N)*radius)), resolution);
		a = min(a,texture(img, c).r + geodistance(st,c,resolution));
	}
	fc = a;
)");
	program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(fragment_set->getCode(), GL_FRAGMENT_SHADER)
			.link();



	Shader* nearest_height = Shader::builder()
			.include(fragmentBase)
			.include(cornerCoords)
			.include(distance)
			.create("uniform float radius; uniform sampler2D target;",R"(
	vec2 resolution = textureSize(img,0);
	float a = texture(img,st).r;
	fc = texture(scratch2,st).r;

	int N = 64;
	for (int i=0; i<N; i++) {
		vec2 c = offset(st, round(vec2(cos(2*3.14159*i/N)*radius,sin(2*3.14159*i/N)*radius)), resolution);

		if (texture(img, c).r + geodistance(st,c,resolution)<a) {
			a = texture(img, c).r + geodistance(st,c,resolution);
			fc = texture(scratch2,c).r;
		}
	}
)");
	height_program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(nearest_height->getCode(), GL_FRAGMENT_SHADER)
			.link();


}

std::pair<bool, float> DeTerrace::step(Project *p) {
	height_program->bind();
	int id = glGetUniformLocation(height_program->getId(), "radius");
	glUniform1f(id,r[steps]);
	p->setCanvasUniforms(height_program);
	p->apply(height_program, p->get_scratch1(),{{target,"target"}});
	p->get_scratch1()->swap(p->get_scratch2());


	program->bind();
	id = glGetUniformLocation(program->getId(), "radius");
	glUniform1f(id,r[steps]);
	p->setCanvasUniforms(program);
	p->apply(program, p->get_scratch1(),{{target,"target"}});
	p->get_scratch1()->swap(target);




	steps++;



	return {steps>=r.size(),(float(steps))/r.size()};
}
