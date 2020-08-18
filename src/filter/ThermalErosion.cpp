//
// Created by kuhlwein on 8/16/20.
//

#include "Project.h"
#include "ThermalErosion.h"

ThermalErosionMenu::ThermalErosionMenu() : FilterModal("Thermal erosion") {

}

void ThermalErosionMenu::update_self(Project *p) {

}

std::shared_ptr<BackupFilter> ThermalErosionMenu::makeFilter(Project *p) {
	auto morph = std::make_unique<ThermalErosion>();
	return std::make_shared<ProgressFilter>(p,[](Project* p){return p->get_terrain();},std::move(morph));
}

ThermalErosion::ThermalErosion() : SubFilter() {

}

std::pair<bool, float> ThermalErosion::step(Project *p) {
	for (int i=0; i<10; i++) {
		Shader *shader = Shader::builder()
				.include(fragmentBase)
				.include(cornerCoords)
				.include(p->canvas->projection_shader())
				.include(get_slope)
				.create("", R"(
	vec2 resolution = textureSize(img,0);
	float slope = get_slope(1,st);
	float h = texture(img,st).r;
	if (slope>M_PI/6 && h>0) {
		float count = 1;

		float minh = h;
		float h2;

		h2 = texture2D(img, offset(st, vec2(-1,1),resolution)).r;
		if (h2<minh) minh=h2;
		if (h2<h) count+=1;
		h2 = texture2D(img, offset(st, vec2(0,1),resolution)).r;
		if (h2<minh) minh=h2;
		if (h2<h) count+=1;
		h2 = texture2D(img, offset(st, vec2(1,1),resolution)).r;
		if (h2<minh) minh=h2;
		if (h2<h) count+=1;
		h2 = texture2D(img, offset(st, vec2(1,0),resolution)).r;
		if (h2<minh) minh=h2;
		if (h2<h) count+=1;
		h2 = texture2D(img, offset(st, vec2(-1,0),resolution)).r;
		if (h2<minh) minh=h2;
		if (h2<h) count+=1;
		h2 = texture2D(img, offset(st, vec2(1,-1),resolution)).r;
		if (h2<minh) minh=h2;
		if (h2<h) count+=1;
		h2 = texture2D(img, offset(st, vec2(0,-1),resolution)).r;
		if (h2<minh) minh=h2;
		if (h2<h) count+=1;
		h2 = texture2D(img, offset(st, vec2(-1,-1),resolution)).r;
		if (h2<minh) minh=h2;
		if (h2<h) count+=1;


		fc = (h-minh)/count * 0.3;
	} else {
		fc = 0;
	}
)");
		ShaderProgram *program = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
				.link();
		program->bind();
		p->setCanvasUniforms(program);
		p->apply(program, p->get_scratch1());
		delete shader;
		delete program;


		shader = Shader::builder()
				.include(fragmentBase)
				.include(cornerCoords)
				.include(get_slope)
				.create("", R"(
	vec2 resolution = textureSize(img,0);
	float h = texture(img,st).r;

	float gain = 0;
	float count = 0;
	float h2;

	h2 = texture2D(img, offset(st, vec2(-1,1),resolution)).r;
	if (h2>h) gain+=texture2D(scratch1,offset(st,vec2(-1,1),resolution)).r;
	if (h2<h) count+=1;
	h2 = texture2D(img, offset(st, vec2(0,1),resolution)).r;
	if (h2>h) gain+=texture2D(scratch1,offset(st,vec2(0,1),resolution)).r;
	if (h2<h) count+=1;
	h2 = texture2D(img, offset(st, vec2(1,1),resolution)).r;
	if (h2>h) gain+=texture2D(scratch1,offset(st,vec2(1,1),resolution)).r;
	if (h2<h) count+=1;
	h2 = texture2D(img, offset(st, vec2(-1,0),resolution)).r;
	if (h2>h) gain+=texture2D(scratch1,offset(st,vec2(-1,0),resolution)).r;
	if (h2<h) count+=1;
	h2 = texture2D(img, offset(st, vec2(1,0),resolution)).r;
	if (h2>h) gain+=texture2D(scratch1,offset(st,vec2(1,0),resolution)).r;
	if (h2<h) count+=1;
	h2 = texture2D(img, offset(st, vec2(-1,-1),resolution)).r;
	if (h2>h) gain+=texture2D(scratch1,offset(st,vec2(-1,-1),resolution)).r;
	if (h2<h) count+=1;
	h2 = texture2D(img, offset(st, vec2(0,-1),resolution)).r;
	if (h2>h) gain+=texture2D(scratch1,offset(st,vec2(0,-1),resolution)).r;
	if (h2<h) count+=1;
	h2 = texture2D(img, offset(st, vec2(1,-1),resolution)).r;
	if (h2>h) gain+=texture2D(scratch1,offset(st,vec2(1,-1),resolution)).r;
	if (h2<h) count+=1;

	float slope = get_slope(1,st);
	if (slope>M_PI/6/10 && h>0) {
		fc = texture(img,st).r;
	} else {
		fc = texture(img,st).r + gain;
}
)");
		program = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
				.link();
		program->bind();
		p->setCanvasUniforms(program);
		p->apply(program, p->get_scratch2());
		p->get_scratch2()->swap(p->get_terrain());
		delete shader;
		delete program;
	}


	return {true,1.0};
}
