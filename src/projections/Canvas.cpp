//
// Created by kuhlwein on 4/30/19.
//

#include <GL/gl3w.h>
#include "Canvas.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <iostream>
#include <imgui/imgui.h>
#include <chrono>
#include "Project.h"
#include "cmath"

Canvas::Canvas(Project *project) {
	this->project = project;
}

float Canvas::windowWidth() {
	return project->getWindowWidth();
}

float Canvas::windowHeight() {
	return project->getWindowHeight();
}

float Canvas::windowAspect() {
	return windowWidth()/windowHeight();
}




AbstractCanvas::AbstractCanvas(Project *project) : Canvas(project) {
	std::vector<float> positions = {
			-1, 1, 0,
			-1, -1, 0,
			1, -1, 0,
			1, 1, 0
	};
	std::vector<float> textures = {
			0.0, 0.0,
			0.0, 1,
			1, 1,
			1, 0
	};
	std::vector<int> indices = {
			0, 1, 3,
			2, 3, 1
	};

	vbo = new Vbo(positions, textures, indices);

	ZOOM = 1.1;
	z = 1;
	x = y = 0;
	rotation = glm::mat4(1.0f);
}

void AbstractCanvas::render() {
	glm::mat4 model(1.f);
	int programId = project->program->getId();

	int id = glGetUniformLocation(programId,"projectionMatrix");
	glUniformMatrix4fv(id,1,GL_FALSE,glm::value_ptr(model));

	id = glGetUniformLocation(programId,"globeRotation");
	glUniformMatrix4fv(id,1,GL_FALSE,glm::value_ptr(rotation));

	id = glGetUniformLocation(programId,"worldMatrix");
	glUniformMatrix4fv(id,1,GL_FALSE,glm::value_ptr(model));

	id = glGetUniformLocation(programId,"u_time");
	glUniform1f(id,(float)(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count())/1000);

	id = glGetUniformLocation(programId,"windowaspect");
	glUniform1f(id,windowAspect());

	id = glGetUniformLocation(programId,"zoom");
	glUniform1f(id,pow(ZOOM,z));

	project->setCanvasUniforms(project->program);

	id = glGetUniformLocation(programId,"isinterrupted");
	glUniform1fv(id, 1, &isinterrupted);
	if (isinterrupted) {
		id = glGetUniformLocation(programId,"interruptions");
		int n = interruptions[0].size()-1;
		glUniform1iv(id, 1, &n);
		id = glGetUniformLocation(programId,"north");
		glUniform1fv(id, n+1, interruptions[0].data());
		id = glGetUniformLocation(programId,"center_north");
		glUniform1fv(id, n, interruptions[1].data());

		id = glGetUniformLocation(programId,"interruptions_s");
		n = interruptions[2].size()-1;
		glUniform1iv(id, 1, &n);
		id = glGetUniformLocation(programId,"south");
		glUniform1fv(id, n+1, interruptions[2].data());
		id = glGetUniformLocation(programId,"center_south");
		glUniform1fv(id, n, interruptions[3].data());
	}

	id = glGetUniformLocation(programId,"xyoffset");
	glUniform2fv(id, 1, std::vector<float>{x,y}.data());

	id = glGetUniformLocation(programId,"scale");
	glUniform2fv(id, 1, std::vector<float>{getScale().x,getScale().y}.data());

	vbo->render();
}

void AbstractCanvas::pan(float dx, float dy) {
	float scaling = (float) (pow(ZOOM,z))*2;

	x-=dx * scaling/windowWidth();
	y-=dy * scaling/windowHeight()/windowAspect();

	float xlim = getLimits().x;
	float ylim = getLimits().y;

	if(x<-xlim) x=-xlim;
	else if(x>xlim) x=xlim;
	if(y<-ylim) y=-ylim;
	else if(y>ylim) y=ylim;
}

void AbstractCanvas::update() {
	ImGuiIO io = ImGui::GetIO();

	if(io.MouseDown[1]) {
		float dx = io.MouseDelta.x, dy=io.MouseDelta.y;
		pan(dx,dy);
	}

	if(io.MouseWheel!=0) {
		float delta = io.MouseWheel;
		z+=delta;
		float deltax = (io.MousePos.x-windowWidth()*0.5f)*(ZOOM-1);
		float deltay = (io.MousePos.y-windowHeight()*0.5f)*(ZOOM-1);
		pan(delta*deltax,delta*deltay);
	}
}

glm::vec2 AbstractCanvas::mousePos(ImVec2 pos) {
	glm::vec2 st = glm::vec2(pos.x/project->getWindowWidth(),pos.y/project->getWindowHeight());

	float x_ = 2.0 * (st.x - 0.5) * pow(ZOOM,z) + x;
	float y_ = 2.0 * (st.y - 0.5 )/windowAspect() * pow(ZOOM,z) + y;

	x_ = x_ * getScale().x;
	y_ = y_ * getScale().y;

	int offset_ = 0;
	float start, stop;
	if (isinterrupted) {
		int offset = y_>0 ? 2 : 0;
		for (int i=0; i<interruptions[offset].size()-1; i++) {
			float x = x_/getScale().x;
			if (x > interruptions[offset][i]/180 && x < interruptions[offset][i+1]/180) {
				start = x<interruptions[offset+1][i]/180 ? interruptions[offset][i]/180 : interruptions[offset+1][i]/180;
				stop = x<interruptions[offset+1][i]/180 ? interruptions[offset+1][i]/180 : interruptions[offset][i+1]/180;
				offset_ = x<interruptions[offset+1][i]/180 ? 1 : 0;
				break;
			}
		}
		x_=(x_-start*getScale().x)/(stop*getScale().x-start*getScale().x)*(getScale().x)+(-getScale().x*(offset_));
	}

	glm::vec2 coord = inverseTransform(glm::vec2(x_,y_));

	if (isinterrupted) {
		coord.x = coord.x>M_PI ? M_PI : coord.x;
		coord.x = coord.x<-M_PI ? -M_PI : coord.x;
		coord.x = (coord.x-(-M_PI*offset_))/(M_PI)*(stop*M_PI-start*M_PI)+start*M_PI;
	}

	glm::vec4 coord2 = rotation*glm::vec4(sin(M_PI/2-coord.y)*cos(coord.x),sin(M_PI/2-coord.y)*sin(coord.x),sin(coord.y),1);
	coord.y = acos(-coord2.z)-0.5*M_PI; //0 to pi
	coord.x = atan2(coord2.y,coord2.x); // -pi to pi

	auto v = project->getCoords();
	float theta = (coord.x-v[2])/(v[3]-v[2]);
	float phi = (coord.y-v[0])/(v[1]-v[0]);
	return glm::vec2(theta,phi);
}

Shader *AbstractCanvas::projection_shader() {
	return Shader::builder()
			.include(def_pi)
			.include(cornerCoords)
			.include(inverseShader())
			.create(R"(
uniform float windowaspect=1.0f;
uniform float zoom = 2;
uniform vec2 xyoffset;
uniform vec2 scale;

uniform mat4 globeRotation;

uniform bool isinterrupted = false;
uniform float north[25];
uniform float center_north[25];
uniform float south[25];
uniform float center_south[25];

uniform int interruptions;
uniform int interruptions_s;

vec2 projection(in vec2 st) {


float x = 2.0 * (st.x - 0.5 )*zoom + xyoffset.x;
float y = 2.0 * (st.y - 0.5 ) / windowaspect * zoom+ xyoffset.y;

x = x*scale.x;
y = y*scale.y;

float start_i;
float stop_i;
float offset = 0.0f;

if (isinterrupted) {
for (int i=0; i<interruptions; i++) {
bool cond = (y<0 && x/scale.x > north[i]/180 && x/scale.x < north[i+1]/180);

start_i = cond && x/scale.x<center_north[i]/180 ? north[i]/180 : start_i;
stop_i = cond && x/scale.x<center_north[i]/180 ? center_north[i]/180 : stop_i;
offset = cond && x/scale.x<center_north[i]/180 ? 0.0f : offset;

start_i = cond && x/scale.x>center_north[i]/180 ? center_north[i]/180 : start_i;
stop_i = cond && x/scale.x>center_north[i]/180 ? north[i+1]/180 : stop_i;
offset = cond && x/scale.x<center_north[i]/180 ? 1.0f : offset;
}
for (int i=0; i<interruptions_s; i++) {
bool cond = (y>0 && x/scale.x > south[i]/180 && x/scale.x < south[i+1]/180);

start_i = cond && x/scale.x<center_south[i]/180 ? south[i]/180 : start_i;
stop_i = cond && x/scale.x<center_south[i]/180 ? center_south[i]/180 : stop_i;
offset = cond && x/scale.x<center_south[i]/180 ? 0.0f : offset;

start_i = cond && x/scale.x>center_south[i]/180 ? center_south[i]/180 : start_i;
stop_i = cond && x/scale.x>center_south[i]/180 ? south[i+1]/180 : stop_i;
offset = cond && x/scale.x<center_south[i]/180 ? 1.0f : offset;
}

x=(x-start_i*scale.x)/(stop_i*scale.x-start_i*scale.x)*(scale.x)+(-scale.x*offset);
}

vec2 coord = inverseshader(vec2(x,y));
float phi = coord.y;
float theta = coord.x;

if (isinterrupted) {
theta = (theta-(-M_PI*offset))/(M_PI)*(stop_i*M_PI-start_i*M_PI)+start_i*M_PI;
}

if (theta<-M_PI) discard;
if (theta>M_PI) discard;
if (phi<-M_PI/2) discard;
if (phi>M_PI/2) discard;

vec4 coord2 = globeRotation*vec4(sin(M_PI/2-phi)*cos(theta),sin(M_PI/2-phi)*sin(theta),sin(phi),1);
phi = acos(-coord2.z)-0.5*M_PI; //0 to pi
theta = atan(coord2.y,coord2.x); // -pi to pi

phi = (phi-cornerCoords[0])/(cornerCoords[1]-cornerCoords[0]);
theta = (theta-cornerCoords[2])/(cornerCoords[3]-cornerCoords[2]);

if (phi<0.0f) discard;
if (phi>1.0f) discard;
if (theta<0.0f) discard;
if (theta>1.0f) discard;

return vec2(theta,phi);
}
)","vec2 st_p = projection(st);");
}

void AbstractCanvas::set_rotation(float theta, float phi, float rho) {
	rotation = glm::rotate(glm::mat4(1.0f), theta, glm::vec3(0, 0, 1))
			* glm::rotate(glm::mat4(1.0f), phi, glm::vec3(0, 1, 0))
			* glm::rotate(glm::mat4(1.0f), rho, glm::vec3(1, 0, 0));
}

void AbstractCanvas::set_interruptions(std::vector<std::vector<float>> interruptions, bool active) {
	this->interruptions = interruptions;
	this->isinterrupted = active ? 1.0f : 0.0f;
}

bool AbstractCanvas::isInterruptible() {
	return false;
}




