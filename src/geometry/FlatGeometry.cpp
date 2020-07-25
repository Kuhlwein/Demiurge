//
// Created by kuhlwein on 7/24/20.
//

#include <glm/ext.hpp>
#include "Project.h"
#include "Geometry.h"
#include "Shader.h"
#include "FlatGeometry.h"

PseudoFlatGeometry::PseudoFlatGeometry(Project *p, std::string brush_code, std::string distance_code, std::string offset_code, std::string free_select_code) : Geometry(p) {
	brush_shader = Shader::builder()
			.include(mouseLocation)
			.create(R"(
uniform sampler2D brush_tex;
uniform mat2 rotation;

void brush_calc(inout vec2 vstart, inout vec2 vstop) {
vec2 pos = st*textureSize(img,0);
vec2 mouse_pos = mouse*textureSize(img,0);
vec2 mousePrev_pos = mousePrev*textureSize(img,0);

pos = pos - mouse_pos;
mousePrev_pos = mousePrev_pos - mouse_pos;

)"+brush_code+R"(

pos = rotation*pos;

mousePrev_pos = rotation*mousePrev_pos;

float d = abs(pos.y);
float width = sqrt(pow(brush_size,2)-pow(d,2));

float rightstart = 0 + width;
rightstart = min(pos.x,rightstart);
rightstart = max(-width,rightstart);

float leftend = mousePrev_pos.x - width;
leftend = max(pos.x,leftend);
leftend = min(mousePrev_pos.x+width,leftend)-mousePrev_pos.x;

float stop = rightstart;
float start = leftend;

vstop = vec2(stop/width/2+0.5,d/brush_size);
vstart = vec2(start/width/2+0.5,d/brush_size);
}
)","");

	distance_shader = Shader::builder().create(R"(
float geodistance(in vec2 p1, in vec2 p2, in vec2 size) {

vec2 diff = abs(p1*size-p2*size);
bvec2 cond = greaterThanEqual(diff,size/2);
)"+distance_code+R"(
return length(vec2(x,y));
}
)","");

	offset_shader = Shader::builder().create(offset_code,"");

	triangle_shader = Shader::builder()
			.include(fragmentBase)
			.include(mouseLocation)
			.create(R"(
uniform vec2 mouseFirst;
bool free_select(vec2 start, vec2 mouse, vec2 prev) {
	vec2 st2 = st;
	)"+free_select_code+R"(
	vec3 AB = vec3(mouse-prev,0);
	vec3 BC = vec3(prev-start,0);
	vec3 CA = vec3(start-mouse,0);
	vec3 AP = vec3(st2-mouse,0);
	vec3 BP = vec3(st2-prev,0);
	vec3 CP = vec3(st2-start,0);

	bool a = AB.x*AP.y>=AB.y*AP.x;
	bool b = BC.x*BP.y>=BC.y*BP.x;
	bool c = CA.x*CP.y>CA.y*CP.x;

	return a == b && b == c;
}
)","");

}

void PseudoFlatGeometry::setup_brush_calc(ShaderProgram *program, glm::vec2 pos, glm::vec2 prev) {
	//todo bind program??
	prev = prev - pos;
	float phi = - atan2(prev.y*p->getHeight(),prev.x*p->getWidth());
	glm::mat2 rotation = glm::mat2(cos(phi), sin(phi), -sin(phi), cos(phi));
	int id = glGetUniformLocation(program->getId(),"rotation");
	glUniformMatrix2fv(id,1,GL_FALSE,glm::value_ptr(rotation));
}

void PseudoFlatGeometry::setup_triangle(ShaderProgram *program, glm::vec2 a, glm::vec2 b, glm::vec2 c) {
	int id = glGetUniformLocation(program->getId(), "mouse");
	glUniform2f(id, b.x, b.y);
	id = glGetUniformLocation(program->getId(), "mousePrev");
	glUniform2f(id, c.x, c.y);
	id = glGetUniformLocation(program->getId(), "mouseFirst");
	glUniform2f(id, a.x, a.y);
}

FlatGeometry::FlatGeometry(Project *p) : PseudoFlatGeometry(p,"",R"(
float x = diff.x;
float y = diff.y;
)",R"(vec2 offset(vec2 p, vec2 dp, vec2 resolution) {
p = p+dp/resolution;
return p;
})","") {

}

WrapXGeometry::WrapXGeometry(Project *p) : PseudoFlatGeometry(p,
															  "pos.x = mod(pos.x+textureSize(img,0).x/2,textureSize(img,0).x)-textureSize(img,0).x/2;",
															  R"(
float x = cond.x ? size.x-diff.x : diff.x;
float y = diff.y;
)",
															  R"(
	vec2 offset(vec2 p, vec2 dp, vec2 resolution) {
p = p+dp/resolution;
p.x = mod(p.x,1);
		return p;
	}
)",R"(
mouse.x = mod(mouse.x-start.x+0.5,1);
prev.x = mod(prev.x-start.x+0.5,1);
st2.x = mod(st2.x - start.x+0.5,1);
start.x = mod(start.x-start.x+0.5,1);
)"
) {

}

WrapYGeometry::WrapYGeometry(Project *p) : PseudoFlatGeometry(p,
															  "pos.y = mod(pos.y+textureSize(img,0).y/2,textureSize(img,0).y)-textureSize(img,0).y/2;",
															  R"(
float y = cond.y ? size.y-diff.y : diff.y;
float x = diff.x;
)",
															  R"(
vec2 offset(vec2 p, vec2 dp, vec2 resolution) {
	p = p+dp/resolution;
    p.y = mod(p.y,1);
    return p;
}
)",R"(
mouse.y = mod(mouse.y-start.y+0.5,1);
prev.y = mod(prev.y-start.y+0.5,1);
st2.y = mod(st2.y - start.y+0.5,1);
start.y = mod(start.y-start.y+0.5,1);
)"
) {

}

WrapXYGeometry::WrapXYGeometry(Project *p) : PseudoFlatGeometry(p,
																R"(
pos.x = mod(pos.x+textureSize(img,0).x/2,textureSize(img,0).x)-textureSize(img,0).x/2;
pos.y = mod(pos.y+textureSize(img,0).y/2,textureSize(img,0).y)-textureSize(img,0).y/2;
)",
																R"(
float x = cond.x ? size.x-diff.x : diff.x;
float y = cond.y ? size.y-diff.y : diff.y;
)",
																R"(
vec2 offset(vec2 p, vec2 dp, vec2 resolution) {
		return mod(p+dp/resolution,1);
	}
)",R"(
mouse = mod(mouse-start+0.5,1);
prev = mod(prev-start+0.5,1);
st2 = mod(st2 - start+0.5,1);
start = mod(start-start+0.5,1);
)"
) {

}

