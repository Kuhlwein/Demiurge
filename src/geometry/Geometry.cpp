//
// Created by kuhlwein on 7/24/20.
//

#include <glm/ext.hpp>
#include "Project.h"
#include "Geometry.h"
#include "Shader.h"

PseudoFlatGeometry::PseudoFlatGeometry(Project *p, std::string brush_code, std::string distance_code, std::string offset_code) : Geometry(p) {
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

	offset_shader = Shader::builder().create(R"(
vec2 offset(vec2 p, vec2 dp, vec2 resolution) {
)"+offset_code+R"(
	return p+dp/resolution;
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

FlatGeometry::FlatGeometry(Project *p) : PseudoFlatGeometry(p,"",R"(
float x = diff.x;
float y = diff.y;
)","") {

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
)"
) {

}

SphericalGeometry::SphericalGeometry(Project *p) : Geometry(p) {
	brush_shader = Shader::builder()
			.include(mouseLocation)
			.include(cornerCoords)
			.include(def_pi)
			.create(R"(
uniform sampler2D brush_tex;
uniform mat4 rotation;

void brush_calc(inout vec2 vstart, inout vec2 vstop) {
  float phi = (st.y)*(cornerCoords[1]-cornerCoords[0])+cornerCoords[0]; //-pi/2 to pi/2
  float theta = st.x*(cornerCoords[3]-cornerCoords[2])+M_PI; // 0 to 2*pi

  vec4 pos = vec4(sin(M_PI/2-phi)*cos(theta),sin(M_PI/2-phi)*sin(theta),cos(M_PI/2-phi),1);
  pos = rotation * pos;
  phi = asin(pos.z);
  theta = atan(pos.y,pos.x);

  float phi_prev = (mousePrev.y)*(cornerCoords[1]-cornerCoords[0])+cornerCoords[0];
  float theta_prev = mousePrev.x*2*M_PI;
  theta_prev = mousePrev.x*(cornerCoords[3]-cornerCoords[2])+M_PI; // 0 to 2*pi

  pos = vec4(sin(M_PI/2-phi_prev)*cos(theta_prev),sin(M_PI/2-phi_prev)*sin(theta_prev),cos(M_PI/2-phi_prev),1);
  pos = rotation * pos;
  phi_prev = asin(pos.z);
  theta_prev = atan(pos.y,pos.x);

  float d = abs(phi)/(cornerCoords[3]-cornerCoords[2])*textureSize(img,0).x;
  float width = sqrt(pow(brush_size,2)-pow(d,2));

  float rightstart = 0 + width;
  rightstart = min(theta,rightstart);
  rightstart = max(-width,rightstart);

  float leftend = theta_prev - width;
  leftend = max(theta,leftend);
  leftend = min(theta_prev+width,leftend)-theta_prev;

  float stop = rightstart/(cornerCoords[3]-cornerCoords[2])*textureSize(img,0).x;
  float start = leftend/(cornerCoords[3]-cornerCoords[2])*textureSize(img,0).x;
  vstop = vec2(stop/width/2+0.5,d/brush_size);
  vstart = vec2(start/width/2+0.5,d/brush_size);
}
)");

	distance_shader = Shader::builder()
			.include(cornerCoords)
			.include(def_pi)
			.create(R"(
float geodistance(vec2 p1, vec2 p2, vec2 size) {
  float dx = abs(p1.x-p2.x)*(cornerCoords[3]-cornerCoords[2]);
  dx = dx>M_PI ? 2*M_PI-dx : dx;
  float y1 = (p1.y)*(cornerCoords[1]-cornerCoords[0])+cornerCoords[0];
  float y2 = (p2.y)*(cornerCoords[1]-cornerCoords[0])+cornerCoords[0];
  float delta_sigma = 2*asin(sqrt( pow(sin(abs(y2-y1)/2) , 2) + cos(y1)*cos(y2)*pow(sin(dx/2),2)));
  return delta_sigma/(cornerCoords[3]-cornerCoords[2])*size.x;
}
)","");

	offset_shader = Shader::builder()
			.create(R"(
vec2 offset(vec2 p, vec2 dp, vec2 resolution) {
p = p + dp/resolution;
p.x = mod(p.x,1);
if (p.y<0) {
p.y=-p.y;
p.x=p.x-0.5;
}
if (p.y>1) {
p.y=2-p.y;
p.x=p.x-0.5;
}
p.x = mod(p.x,1);
return p;
}
)","");
}

void SphericalGeometry::setup_brush_calc(ShaderProgram *program, glm::vec2 pos, glm::vec2 prev) {
	auto v = p->getCoords();
	for (auto &e : v) e=e/180.0f*M_PI;

	glm::mat4 rotationo(1.f);
	glm::mat4 rotation(1.f);
	float dtheta = -pos.x*(v[3]-v[2])+M_PI;
	rotation = glm::rotate(rotationo, dtheta, glm::vec3(0, 0, 1));
	float dphi = (pos.y)*(v[1]-v[0])+v[0];
	rotation = glm::rotate(rotationo, dphi, glm::vec3(0, 1, 0))*rotation;

	float phi = (prev.y-0.5)*M_PI;
	phi = (prev.y)*(v[1]-v[0])+v[0];
	float theta = prev.x*(v[3]-v[2])+M_PI;

	glm::vec4 pos_ = glm::vec4(sin(M_PI/2-phi)*cos(theta),sin(M_PI/2-phi)*sin(theta),cos(M_PI/2-phi),1);
	glm::vec4 prev_ = rotation*pos_;
	dtheta = -atan2(prev_.z,prev_.y);
	rotation = glm::rotate(rotationo, dtheta, glm::vec3(1, 0, 0))*rotation;

	int id = glGetUniformLocation(program->getId(),"rotation");
	glUniformMatrix4fv(id,1,GL_FALSE,glm::value_ptr(rotation));

	id = glGetUniformLocation(program->getId(),"cornerCoords");

	glUniform1fv(id, 4, v.data());
}
