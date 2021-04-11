//
// Created by kuhlwein on 7/2/20.
//

#include <GL/gl3w.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <iostream>
#include <imgui/imgui.h>
#include <chrono>
#include "Project.h"
#include "cmath"
#include "Orthographic.h"

Orthographic::Orthographic(Project *p) : Canvas(p) {
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

}

void Orthographic::render() {
	glm::mat4 model(1.f);
	glm::mat4 rotation(1.f);
	rotation = glm::rotate(rotation, delta_theta, glm::vec3(0, 0, 1)) * glm::rotate(rotation, delta_phi, glm::vec3(1, 0, 0));

	int programId = project->program->getId();

	int id = glGetUniformLocation(programId,"projectionMatrix");
	glUniformMatrix4fv(id,1,GL_FALSE,glm::value_ptr(model));

	id = glGetUniformLocation(programId,"worldMatrix");
	glUniformMatrix4fv(id,1,GL_FALSE,glm::value_ptr(model));

	id = glGetUniformLocation(programId,"u_time");
	glUniform1f(id,(float)(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count())/1000);


	id = glGetUniformLocation(programId,"windowaspect");
	glUniform1f(id,windowAspect());

	id = glGetUniformLocation(programId,"globeRotation");
	glUniformMatrix4fv(id,1,GL_FALSE,glm::value_ptr(rotation));

	id = glGetUniformLocation(programId,"zoom");
	glUniform1f(id,pow(ZOOM,z));

	project->setCanvasUniforms(project->program);

	vbo->render();
}

void Orthographic::update() {
	auto io = ImGui::GetIO();

	if(io.MouseDown[1]) {
		auto mouse = io.MousePos;
		auto delta = io.MouseDelta;

		auto pos = mousePos(mouse);
		auto oldpos = mousePos(ImVec2(mouse.x-delta.x,mouse.y-delta.y));
		pos = pos-oldpos;

		if (!(std::isnan(pos.x) || std::isnan(pos.y))) {
			auto v = project->getCoords();
			delta_phi += pos.y*(v[1]-v[0]);
			delta_theta -= pos.x*(v[3]-v[2]);
			if (delta_phi < 0) delta_phi = 0;
			if (delta_phi > M_PI) delta_phi = M_PI;
		}
	}

	if(io.MouseWheel!=0) {
		float delta = io.MouseWheel;

		z+=delta;
	}
}

glm::vec2 Orthographic::mousePos(ImVec2 pos) {
	glm::vec2 st = glm::vec2(pos.x/project->getWindowWidth(),pos.y/project->getWindowHeight());

	float x = 2.0 * (st.x - 0.5) * pow(ZOOM,z);
	float y = 2.0 * (st.y - 0.5)/windowAspect() * pow(ZOOM,z);
	float r = sqrt(x * x + y * y);

	float z = sqrt(1-r*r);

	glm::mat4 globeRotation(1.f);
	globeRotation = glm::rotate(globeRotation, delta_theta, glm::vec3(0, 0, 1)) * glm::rotate(globeRotation, delta_phi, glm::vec3(1, 0, 0));
	glm::vec4 coord = globeRotation * glm::vec4(x, y, z, 1);

	float phi = acos(-coord.z)-M_PI/2; //-pi/2 to pi/2
	float theta = atan2(coord.y,coord.x); // -pi to pi

	auto v = project->getCoords();

	float phi2 = (phi-v[0])/(v[1]-v[0]);
	float theta2 = (fmod(theta+M_PI-v[2],M_PI*2))/(v[3]-v[2]);

	return glm::vec2(theta2,phi2);
}

Shader *Orthographic::projection_shader() {
	return Shader::builder()
			.include(def_pi)
			.include(cornerCoords)
			.create(R"(
uniform float windowaspect=1.0f;
uniform mat4 globeRotation;

uniform float zoom = 2;

vec2 projection(in vec2 st, inout bool outOfBounds) {

float x = 2.0 * (st.x - 0.5)*zoom;
float y = 2.0 * (st.y - 0.5) / windowaspect * zoom;
float r = sqrt(x * x + y * y);

float z = sqrt(1-r*r);

vec4 coord = globeRotation*vec4(x,y,z,1);
float r2 = sqrt(coord.x*coord.x+coord.y*coord.y);

if (r>1) outOfBounds=true;

float phi = -asin(-coord.z); //0 to pi
float theta = atan(coord.y,coord.x); // -pi to pi


float phi2 = (phi-cornerCoords[0])/(cornerCoords[1]-cornerCoords[0]); // 0 to 1
float theta2 = mod(theta,2*M_PI); // 0 to 2*pi
theta2 = (theta2-cornerCoords[2]-M_PI)/(cornerCoords[3]-cornerCoords[2]);

if (phi2<0.0f) outOfBounds=true;
if (phi2>1.0f) outOfBounds=true;
if (theta2<0.0f) outOfBounds=true;
if (theta2>1.0f) outOfBounds=true;


return vec2(theta2,phi2);
}

vec2 projection(in vec2 st) {
	bool a;
	vec2 r = projection(st,a);
	if(a) discard;
	return r;
}

)","vec2 st_p = projection(st);");
}