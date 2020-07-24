//
// Created by kuhlwein on 7/24/20.
//

#include <glm/ext.hpp>
#include "Project.h"
#include "Geometry.h"
#include "Shader.h"
#include "SphericalGeometry.h"

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

	triangle_shader = Shader::builder()
			.include(fragmentBase)
			.include(mouseLocation)
			.include(def_pi)
			.create(R"(
uniform vec2 mouseFirst;
bool free_select(vec2 start, vec2 mouse, vec2 prev) {
float phi = (mouse.y*2-1)*M_PI/2;
float theta = (mouse.x*2-1)*M_PI;
vec3 A = vec3(sin(M_PI/2-phi)*cos(theta),sin(M_PI/2-phi)*sin(theta),cos(M_PI/2-phi));

phi = (mousePrev.y*2-1)*M_PI/2;
theta = (mousePrev.x*2-1)*M_PI;
vec3 B = vec3(sin(M_PI/2-phi)*cos(theta),sin(M_PI/2-phi)*sin(theta),cos(M_PI/2-phi));

phi = (mouseFirst.y*2-1)*M_PI/2;
theta = (mouseFirst.x*2-1)*M_PI;
vec3 C = vec3(sin(M_PI/2-phi)*cos(theta),sin(M_PI/2-phi)*sin(theta),cos(M_PI/2-phi));

phi = (st.y*2-1)*M_PI/2;
theta = (st.x*2-1)*M_PI;
vec3 P = vec3(sin(M_PI/2-phi)*cos(theta),sin(M_PI/2-phi)*sin(theta),cos(M_PI/2-phi));

vec3 average = (A+B+C);

vec3 a = cross(A,B);
vec3 b = cross(B,C);
vec3 c = cross(C,A);
//same sign as P!

float s = sign(dot(a,average));

return s*dot(a,P)>0 && s*dot(b,P)>0 && s*dot(c,P)>0;
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

void SphericalGeometry::setup_triangle(ShaderProgram *program, glm::vec2 a, glm::vec2 b, glm::vec2 c) {
	int id = glGetUniformLocation(program->getId(), "mouse");
	glUniform2f(id, b.x, b.y);
	id = glGetUniformLocation(program->getId(), "mousePrev");
	glUniform2f(id, c.x, c.y);
	id = glGetUniformLocation(program->getId(), "mouseFirst");
	glUniform2f(id, a.x, a.y);
}