//
// Created by kuhlwein on 4/16/20.
//
#include <glm/ext.hpp>
#include <iostream>
#include "Project.h"
#include "edit.h"


bool edit::undo(Project* p) {
	p->undo();
	return true;
}

bool edit::redo(Project* p) {
	p->redo();
	return true;
}

bool edit::preferences(Project* p) {
	static bool first = true;
	static float begin, end, begin2, end2;

	if (first) {
		first = false;
		auto v = p->getCoords();
		begin = v[0];
		end = v[1];
		begin2 = v[2];
		end2 = v[3];
	}

	static int current = 0;
	const char* items[] = {"Flat","Wrap x-axis", "Wrap y-axis","Toroidal","Spherical"};
	if(ImGui::Combo("Canvas geometry",&current,items,IM_ARRAYSIZE(items))) {

	}

	ImGui::DragFloatRange2("Lattitude", &begin, &end, 0.25f, -90.0f, 90.0f, "Min: %.1f", "Max: %.1f");
	ImGui::DragFloatRange2("Longitude", &begin2, &end2, 0.25f, -180.0f, 180.0f, "Min: %.1f", "Max: %.1f");

	if (ImGui::Button("Apply")) {
		switch (current) {
			case 0: { p->setGeometryShader(new NoneShader(p)); break;}
			case 1: { p->setGeometryShader(new XShader(p)); break;}
			case 2: { p->setGeometryShader(new YShader(p)); break;}
			case 3: { p->setGeometryShader(new XYShader(p)); break;}
			case 4: { p->setGeometryShader(new SphericalShader(p)); break;}
		}
		first = true;
		p->setCoords({begin,end,begin2,end2});
		return true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel")) {
		first = true;
		return true;
	}
	return false;
}

GeometryShader::GeometryShader(Project *p) {
	this->p = p;
}
Shader *GeometryShader::get_shader() {
	return nullptr;
}
std::function<void(glm::vec2 pos, glm::vec2 prev, ShaderProgram *program)> GeometryShader::get_setup() {
	return std::function<void(glm::vec2, glm::vec2, ShaderProgram *)>();
}




SphericalShader::SphericalShader(Project *p) : GeometryShader(p) {}
Shader *SphericalShader::get_shader() {
	std::string brush_calc = R"(
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
)";
	std::string geodistance = R"(
float geodistance(vec2 p1, vec2 p2, vec2 size) {
  float dx = abs(p1.x-p2.x)*(cornerCoords[3]-cornerCoords[2]);
  dx = dx>M_PI ? 2*M_PI-dx : dx;
  float y1 = (p1.y)*(cornerCoords[1]-cornerCoords[0])+cornerCoords[0];
  float y2 = (p2.y)*(cornerCoords[1]-cornerCoords[0])+cornerCoords[0];
  float delta_sigma = 2*asin(sqrt( pow(sin(abs(y2-y1)/2) , 2) + cos(y1)*cos(y2)*pow(sin(dx/2),2)));
  return delta_sigma/(cornerCoords[3]-cornerCoords[2])*size.x;
}
)";
	std::string offset = R"(
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
)";
	return Shader::builder()
			.include(def_pi)
			.include(cornerCoords)
			.include(mouseLocation)
			.create(brush_calc+geodistance+offset,"");
}

std::function<void(glm::vec2 pos, glm::vec2 prev, ShaderProgram *program)> SphericalShader::get_setup() {
	auto project = p;
	return [project](glm::vec2 pos, glm::vec2 prev, ShaderProgram* program) {
		auto v = project->getCoords();
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
	};
}

auto gen_geodistance = [](std::string s) {
	return R"(
float geodistance(in vec2 p1, in vec2 p2, in vec2 size) {

vec2 diff = abs(p1*size-p2*size);
bvec2 cond = greaterThanEqual(diff,size/2);
)"+s+R"(
return length(vec2(x,y));
}
)";
};
auto gen_brush_calc = [](std::string s) {
	return R"(
uniform sampler2D brush_tex;
uniform mat2 rotation;

void brush_calc(inout vec2 vstart, inout vec2 vstop) {
vec2 pos = st*textureSize(img,0);
vec2 mouse_pos = mouse*textureSize(img,0);
vec2 mousePrev_pos = mousePrev*textureSize(img,0);

pos = pos - mouse_pos;
mousePrev_pos = mousePrev_pos - mouse_pos;

)"+s+R"(

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
)";
};

NoneShader::NoneShader(Project *p) : GeometryShader(p) {}
Shader *NoneShader::get_shader() {
	std::string brush_calc = gen_brush_calc("");
	std::string geodistance = gen_geodistance(R"(
float x = diff.x;
float y = diff.y;
)");
	std::string offset = R"(
	vec2 offset(vec2 p, vec2 dp, vec2 resolution) {
		return p+dp/resolution;
	}
)";
	return Shader::builder()
			.include(mouseLocation)
			.create(brush_calc+geodistance+offset,"");

}
std::function<void(glm::vec2 pos, glm::vec2 prev, ShaderProgram *program)> NoneShader::get_setup() {
	auto project = p;
	return [project](glm::vec2 pos, glm::vec2 prev, ShaderProgram* program) {
		prev = prev - pos;
		float phi = - atan2(prev.y*project->getHeight(),prev.x*project->getWidth());
		glm::mat2 rotation = glm::mat2(cos(phi), sin(phi), -sin(phi), cos(phi));
		int id = glGetUniformLocation(program->getId(),"rotation");
		glUniformMatrix2fv(id,1,GL_FALSE,glm::value_ptr(rotation));
	};
}

Shader *XShader::get_shader() {
	std::string brush_calc = gen_brush_calc(R"(
pos.x = mod(pos.x+textureSize(img,0).x/2,textureSize(img,0).x)-textureSize(img,0).x/2;
)");
	std::string geodistance = gen_geodistance(R"(
float x = cond.x ? size.x-diff.x : diff.x;
float y = diff.y;
)");
	std::string offset = R"(
	vec2 offset(vec2 p, vec2 dp, vec2 resolution) {
p = p+dp/resolution;
p.x = mod(p.x,1);
		return p;
	}
)";
	return Shader::builder()
			.include(mouseLocation)
			.create(brush_calc+geodistance+offset,"");
}

Shader *YShader::get_shader() {
	std::string brush_calc = gen_brush_calc(R"(
pos.y = mod(pos.y+textureSize(img,0).y/2,textureSize(img,0).y)-textureSize(img,0).y/2;
)");
	std::string geodistance = gen_geodistance(R"(
float y = cond.y ? size.y-diff.y : diff.y;
float x = diff.x;
)");
	std::string offset = R"(
vec2 offset(vec2 p, vec2 dp, vec2 resolution) {
	p = p+dp/resolution;
p.y = mod(p.y,1);
		return p;
	}
)";
	return Shader::builder()
			.include(mouseLocation)
			.create(brush_calc+geodistance+offset,"");
}

Shader *XYShader::get_shader() {
	std::string brush_calc = gen_brush_calc(R"(
pos.x = mod(pos.x+textureSize(img,0).x/2,textureSize(img,0).x)-textureSize(img,0).x/2;
pos.y = mod(pos.y+textureSize(img,0).y/2,textureSize(img,0).y)-textureSize(img,0).y/2;
)");
	std::string geodistance = gen_geodistance(R"(
float x = cond.x ? size.x-diff.x : diff.x;
float y = cond.y ? size.y-diff.y : diff.y;
)");
	std::string offset = R"(
	vec2 offset(vec2 p, vec2 dp, vec2 resolution) {
		return mod(p+dp/resolution,1);
	}
)";
	return Shader::builder()
			.include(mouseLocation)
			.create(brush_calc+geodistance+offset,"");
}


