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

Equiretangular::Equiretangular(Project* project) : Canvas(project) {
    //this->project = project;
    canvasAspect = (float) project->getWidth() / project->getHeight();

    std::vector<float> positions = {
            -canvasAspect, 1, 0,
            -canvasAspect, -1, 0,
            canvasAspect, -1, 0,
            canvasAspect, 1, 0
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
    z=0;
    x=0; y=0;

    FOVY = glm::radians(60.0); //radian
    TANFOV = glm::tan(FOVY *0.5);

    windowAspect = 1.7777;
    Z_NEAR = 0.001f;
    Z_FAR = 1000.f;
    ZOOM = 1.1;
}

void Equiretangular::render() {
    glm::mat4 model(1.f);
    glm::mat4 projection = glm::perspective(FOVY,windowAspect,Z_NEAR,Z_FAR);
    glm::mat4 world = glm::translate(model,glm::vec3(x,y,-pow(ZOOM,z)));

    int programId = project->program->getId();

    int id = glGetUniformLocation(programId,"projectionMatrix");
    glUniformMatrix4fv(id,1,GL_FALSE,glm::value_ptr(projection));

    id = glGetUniformLocation(programId,"worldMatrix");
    glUniformMatrix4fv(id,1,GL_FALSE,glm::value_ptr(world));

    id = glGetUniformLocation(programId,"u_time");
    glUniform1f(id,(float)(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count())/1000);

    vbo->render();
}

Equiretangular::~Equiretangular() {
    delete(vbo);
}

void Equiretangular::pan(float dx, float dy) {
    float scaling = (float) (pow(ZOOM,z)+Z_NEAR)*TANFOV*2/windowHeight;
    x+=dx * scaling;
    y-=dy * scaling;

    if(x<-canvasAspect) x=-canvasAspect;
    else if(x>canvasAspect) x=canvasAspect;
    if(y<-1) y=-1;
    else if(y>1) y=1;
}

glm::vec2 Equiretangular::mousePos(ImVec2 pos) {
    glm::mat4 model(1.f);
    glm::mat4 projection = glm::perspective(FOVY,windowAspect,Z_NEAR,Z_FAR);
    glm::mat4 world = glm::translate(model,glm::vec3(x,y,-pow(ZOOM,z)));


    glm::vec2 viewpoint(pos.x,pos.y);

    glm::vec4 normalized(2*viewpoint.x/windowWidth-1,-(2*viewpoint.y/windowHeight-1),-1,1); //REVERSED

    glm::vec4 unprojected = glm::inverse(projection) * normalized;


    unprojected = unprojected / unprojected.w;

    glm::vec3 unprojected3(unprojected.x,unprojected.y,unprojected.z);

    glm::vec3 worldvec = glm::inverse(world) * unprojected;

    glm::vec3 ray = unprojected3 / (unprojected3.z) * (-(float) pow(ZOOM,z));
    ray = glm::vec3(ray.x-x,ray.y-y,ray.z);

    glm::vec2 texcoord((ray.x+canvasAspect)/(2*canvasAspect),-(ray.y-1)/2);
    return texcoord;
}

void Equiretangular::update() {
    ImGuiIO io = ImGui::GetIO();
    windowHeight = project->getWindowHeight();
    windowWidth = project->getWindowWidth();
    windowAspect = (float)windowWidth/windowHeight;

    if(io.WantCaptureMouse) return;

    if(io.MouseDown[1]) {
        float dx = io.MouseDelta.x, dy=io.MouseDelta.y;
        pan(dx,dy);
    }


    if(io.MouseWheel!=0) {
        float delta = io.MouseWheel;

        z+=delta;
        float deltax = (io.MousePos.x-windowWidth*0.5f)*(ZOOM-1);
        float deltay = (io.MousePos.y-windowHeight*0.5f)*(ZOOM-1);
        pan(delta*deltax,delta*deltay);

    }
}

Shader *Equiretangular::projection_shader() {
	return Shader::builder().create(R"(
vec2 projection(in vec2 st) {
    return st;
}
)","vec2 st_p = projection(st);");
}






AbstractCanvas::AbstractCanvas(Project *project, glm::vec2 scale) : Canvas(project), scale(scale) {
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

	windowAspect = 1.7777;
}

void AbstractCanvas::render() {
	glm::mat4 model(1.f);
	int programId = project->program->getId();

	int id = glGetUniformLocation(programId,"projectionMatrix");
	glUniformMatrix4fv(id,1,GL_FALSE,glm::value_ptr(model));

	id = glGetUniformLocation(programId,"worldMatrix");
	glUniformMatrix4fv(id,1,GL_FALSE,glm::value_ptr(model));

	id = glGetUniformLocation(programId,"u_time");
	glUniform1f(id,(float)(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count())/1000);

	id = glGetUniformLocation(programId,"windowaspect");
	glUniform1f(id,windowAspect);

	id = glGetUniformLocation(programId,"zoom");
	glUniform1f(id,pow(ZOOM,z));

	id = glGetUniformLocation(programId,"cornerCoords");
	auto v = project->getCoords();
	for (auto &e : v) e=e/180.0f*M_PI;
	glUniform1fv(id, 4, v.data());

	id = glGetUniformLocation(programId,"xyoffset");
	glUniform2fv(id, 1, std::vector<float>{x,y}.data());


	vbo->render();
}

void AbstractCanvas::pan(float dx, float dy) {
	float scaling = (float) (pow(ZOOM,z))*2;

	x-=dx * scaling/windowWidth;
	y-=dy * scaling/windowHeight/windowAspect;

	float xlim = 1.0;
	float ylim = 0.5;

	if(x<-xlim) x=-xlim;
	else if(x>xlim) x=xlim;
	if(y<-ylim) y=-ylim;
	else if(y>ylim) y=ylim;
	
}

void AbstractCanvas::update() {
	ImGuiIO io = ImGui::GetIO();
	windowHeight = project->getWindowHeight();
	windowWidth = project->getWindowWidth();
	windowAspect = (float)windowWidth/windowHeight;

	if(io.WantCaptureMouse) return;

	if(io.MouseDown[1]) {
		float dx = io.MouseDelta.x, dy=io.MouseDelta.y;
		pan(dx,dy);
	}

	if(io.MouseWheel!=0) {
		float delta = io.MouseWheel;

		z+=delta;
		float deltax = (io.MousePos.x-windowWidth*0.5f)*(ZOOM-1);
		float deltay = (io.MousePos.y-windowHeight*0.5f)*(ZOOM-1);
		pan(delta*deltax,delta*deltay);

	}
}

glm::vec2 AbstractCanvas::mousePos(ImVec2 pos) {
	glm::vec2 st = glm::vec2(pos.x/project->getWindowWidth(),pos.y/project->getWindowHeight());

	float x_ = 2.0 * (st.x - 0.5) * pow(ZOOM,z) + x;
	float y_ = 2.0 * (st.y - 0.5 )/windowAspect * pow(ZOOM,z) + y;

	x_ = x_ * scale.x;
	y_ = y_ * scale.y;

	glm::vec2 coord = inverseTransform(glm::vec2(x_,y_));

	auto v = project->getCoords();
	for (auto &e : v) e=e/180.0f*M_PI;
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

vec2 projection(in vec2 st) {

float x = 2.0 * (st.x - 0.5 )*zoom+ xyoffset.x;
float y = 2.0 * (st.y - 0.5 ) / windowaspect * zoom+ xyoffset.y;

x = x*)" + std::to_string(scale.x) + ";" + "y = y*" + std::to_string(scale.y) + ";" + R"(

vec2 coord = inverseshader(vec2(x,y));
float phi = coord.y;
float theta = coord.x;

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
