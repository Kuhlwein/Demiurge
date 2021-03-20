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

	pidShader = Shader::builder()
			.create(R"(
	uint coordToPid(vec2 st, sampler2D img) {
		vec2 coord = textureSize(img,0)*st;
		return uint(coord.x)+uint(coord.y)*uint(textureSize(img,0).x);
	}

	vec2 pidToCoord(uint pid, sampler2D img) {
		ivec2 size = textureSize(img,0);
		uint a = pid - size.x*(pid/size.x);
		uint b = (pid - a)/size.x;
		return vec2(float(a)+0.5,float(b)+0.5)/size;
	}
)","");

	/*
	 * scratch 2 -> pid
	 */
	Shader* shader = Shader::builder()
			.include(fragmentBase)
			.include(pidShader)
			.create("",R"(
	fc =  uintBitsToFloat(coordToPid(st,img));
)");
	program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();

	program->bind();
	p->setCanvasUniforms(program);
	p->apply(program,p->get_scratch2());

	/*
	 * scratch 2 -> if neightbour: neighbour else self
	 */

	shader = Shader::builder()
			.include(fragmentBase)
			.include(cornerCoords)
			.include(pidShader)
			.create("",R"(
	vec2 resolution = textureSize(img,0);
	vec2 o = vec2(1,1);
	float a;
	if (texture(img,offset(st,o,resolution)).r == texture(img,st).r) {
		a = texture(scratch2,st).r;
	} else {
		a = texture(scratch2,offset(st,o,resolution)).r;
	}
	fc = a;
)");
	program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();
	program->bind();
	p->setCanvasUniforms(program);
	p->apply(program,p->get_scratch1());
	p->get_scratch1()->swap(p->get_scratch2());




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


	/*
	 * if same height:
	 * 		check distance, but only if not self.
	 * if different height (Dont do this, do a pre-scan)
	 * 		check distance
	 */

	Shader* fragment_set = Shader::builder()
			.include(fragmentBase)
			.include(cornerCoords)
			.include(distance)
			.include(pidShader)
			.create("uniform float radius;",R"(
	vec2 resolution = textureSize(img,0);

	float min_d = -1.0;
	uint pid = floatBitsToUint(texture(scratch2,st).r);

	if (coordToPid(st,scratch2) != pid) {
		min_d = geodistance(st,pidToCoord(pid,img),resolution);
	}

	float d;
	uint newPid;
	vec2 o;

	o = vec2(radius,0);
	newPid = floatBitsToUint(texture(scratch2,offset(st,o,resolution)).r);
	d = texture(img,pidToCoord(newPid,img)).r;

	if (d != texture(img,st).r && (min_d<0 || geodistance(st,pidToCoord(newPid,img),resolution)<min_d)) {
		min_d = geodistance(st,pidToCoord(newPid,img),resolution);
		pid = newPid;
	}

	//o = vec2(radius,radius);
	//d = max(d,texture(img,offset(st,o,resolution)).r);
	o = vec2(radius,radius);
	newPid = floatBitsToUint(texture(scratch2,offset(st,o,resolution)).r);
	d = texture(img,pidToCoord(newPid,img)).r;

	if (d != texture(img,st).r && (min_d<0 || geodistance(st,pidToCoord(newPid,img),resolution)<min_d)) {
		min_d = geodistance(st,pidToCoord(newPid,img),resolution);
		pid = newPid;
	}

	fc = uintBitsToFloat(pid);
)");
	program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(fragment_set->getCode(), GL_FRAGMENT_SHADER)
			.link();
}

std::pair<bool, float> DeTerrace::step(Project *p) {
	auto aa = (target->downloadDataRAW()); //TODO better solution than casting
	std::cout << ((int*)aa.get())[0] << " <<<\n";




	int a = std::ceil(log2(std::max(p->getHeight(),p->getWidth())));
	for (int i=0; i<=a-3; i++) {
		program->bind();
		int id = glGetUniformLocation(program->getId(), "radius");
		glUniform1f(id,pow(2,i));
		std::cout << "radius: " << pow(2,i) << "\n";
		p->setCanvasUniforms(program);

		p->apply(program, p->get_scratch1());
		p->get_scratch1()->swap(p->get_scratch2());

	}
	for (int i=a-3; i>=0; i--) {
		program->bind();
		int id = glGetUniformLocation(program->getId(), "radius");
		glUniform1f(id,pow(2,i));
		p->setCanvasUniforms(program);

		p->apply(program, p->get_scratch1());
		p->get_scratch1()->swap(p->get_scratch2());
	}





	Shader* fragment_set = Shader::builder()
			.include(fragmentBase)
			.include(cornerCoords)
			.include(distance)
			.include(pidShader)
			.create("uniform float radius;",R"(
	vec2 resolution = textureSize(img,0);

	uint pid = floatBitsToUint(texture(scratch2,st).r);

	fc = float(pid);
	//if (pid == coordToPid(st,img)) fc = -1;
	//fc = geodistance(st,pidToCoord(pid,img),resolution);
)");
	program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(fragment_set->getCode(), GL_FRAGMENT_SHADER)
			.link();
	program->bind();
	int id = glGetUniformLocation(program->getId(), "radius");
	glUniform1f(id,pow(2,i));
	p->setCanvasUniforms(program);
	p->apply(program, p->get_scratch1());
	p->get_scratch1()->swap(p->get_scratch2());



	steps++;

	target->swap(p->get_scratch2());
	return {true,1};

	return {steps>=r.size(),(float(steps))/r.size()};
}
