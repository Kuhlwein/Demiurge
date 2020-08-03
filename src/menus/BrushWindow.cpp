//
// Created by kuhlwein on 7/13/20.
//

#include <imgui/imgui.h>
#include <iostream>
#include <glm/glm/ext.hpp>
#include "BrushWindow.h"
#include "Project.h"

BrushWindow::BrushWindow(std::string title, Project* p) : Window(title, [](Project* p){return true;}) {
	brush_tex = new Texture(brush_tex_size,brush_tex_size,GL_R32F,"brush_tex",GL_LINEAR);
	p->add_texture(brush_tex);
	set_hardness(0.5f);
}

bool BrushWindow::update(Project *p) {
	if (isOpen) {
		ImGui::Begin(title.c_str(), &isOpen,ImGuiWindowFlags_AlwaysAutoResize);
		if (brush_window(p)) isOpen=false;
		ImGui::End();
	}
	handle_brush(p);
	return isOpen;
}

bool BrushWindow::brush_window(Project *p) {
	if (ImGui::DragFloat("Value",&value,0.1f,0.0f,0.0f,"%.2f",1.0f)) {
	}

	if (ImGui::DragFloat("Size",&brush_size,1.0f,0.0f,FLT_MAX,"%.2f",1.0f)) {
	}

	if (ImGui::DragFloat("Hardness",&hardness,0.01f,0.0f,1.0f,"%.2f",1.0f)) {
		set_hardness(hardness);
	}

	ImGui::DragFloat("flow",&flow,0.01f,0.0f,FLT_MAX,"%.2f",1.0f);

	if (limitEnabled) {
		ImGui::DragFloat("##limit",&limit,0.01f,0.0f,FLT_MAX,"%.2f",1.0f);
		ImGui::SameLine();
	}
	ImGui::Checkbox("Limit",&limitEnabled);

	return false;
}

//Handle drawing of brush stroke
void BrushWindow::handle_brush(Project *p) {
	ImGuiIO io = ImGui::GetIO();

	static bool first = true;

	glm::vec2 texcoord = p->getMouse();
	glm::vec2 texcoordPrev = p->getMousePrev();

	auto program = p->program;
	program->bind();

	//TODO brush window should not be responsible for mouse position!
	int id = glGetUniformLocation(program->getId(),"mouse");
	glUniform2f(id,texcoord.x,texcoord.y);
	id = glGetUniformLocation(program->getId(),"mousePrev");
	glUniform2f(id,texcoordPrev.x,texcoordPrev.y);
	id = glGetUniformLocation(program->getId(),"brush_size");
	glUniform1f(id,brush_size);

	if(io.WantCaptureMouse) return;

	if(io.MouseDown[0] & first) {
		initbrush(p);
		first = false;
	} else if (io.MouseDown[0]) {
		brush(p,texcoord,texcoordPrev);
	} else if (!first) {
		Shader *img_tmp_diff = Shader::builder()
				.include(fragmentBase)
				.create("uniform sampler2D target;", R"(
fc = texture(scratch1,st).r - texture(target, st).r;
)");
		// find difference between backup and target
		ShaderProgram *program2 = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(img_tmp_diff->getCode(), GL_FRAGMENT_SHADER)
				.link();

		Texture* tmp = new Texture(p->getWidth(),p->getHeight(),GL_R32F,"tmp");
		p->apply(program2, tmp,{{p->get_terrain(),"target"}});

		p->addAsyncTex(tmp);

		first = true;
	}
}

//Initialize brush for sequential brush strokes
void BrushWindow::initbrush(Project* p) {
	ShaderProgram *program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(copy_img->getCode(), GL_FRAGMENT_SHADER)
			.link();
	p->apply(program,p->get_scratch1(),{{p->get_terrain(),"to_be_copied"}});

	ShaderProgram *program2 = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(fragment_set->getCode(), GL_FRAGMENT_SHADER)
			.link();
	p->apply(program2,p->get_scratch2());
}

//Do single segment brush stroke
void BrushWindow::brush(Project* p, glm::vec2 pos, glm::vec2 prev) {
	//Paint to scratchpad2

	Shader* brush_calc_shader = Shader::builder()
			.include(mouseLocation)
			.include(cornerCoords)
			.include(def_pi)
			.create(R"(
uniform sampler2D brush_tex;
uniform mat4 rotation;

void brush_calc(inout vec2 vstart, inout vec2 vstop) {
	vec2 p = tex_to_spheric(st);
  	vec4 pos = spheric_to_cartesian(p);
  	pos = rotation * pos;
	p = cartesian_to_spheric(pos);

	vec2 m = tex_to_spheric(mousePrev);
  	pos = spheric_to_cartesian(m);
  	pos = rotation * pos;
	m = cartesian_to_spheric(pos);

	float texture_to_sphere_factor = textureSize(img,0).x/(cornerCoords[3]-cornerCoords[2]);
  	float d = abs(p.y)*texture_to_sphere_factor;
  	float width = sqrt(pow(brush_size,2)-pow(d,2));

  	float rightstart = 0 + width;
  	rightstart = min(p.x,rightstart);
  	rightstart = max(-width,rightstart);

  	float leftend = m.x - width;
  	leftend = max(p.x,leftend);
  	leftend = min(m.x+width,leftend)-m.x;

  	float stop = rightstart*texture_to_sphere_factor;
  	float start = leftend*texture_to_sphere_factor;
  	vstop = vec2(stop/width/2+0.5,d/brush_size);
  	vstart = vec2(start/width/2+0.5,d/brush_size);
}
)");

	static Shader* brush_shader = Shader::builder()
			.include(fragmentBase)
			.include(brush_calc_shader)
			.create(R"(
uniform float brush_flow;
)",R"(
vec2 vstop;
vec2 vstart;
brush_calc(vstart,vstop);
fc = texture(scratch2,st).r + brush_flow*(texture(sel,st).r*(texture(brush_tex,vstop).r - texture(brush_tex,vstart).r));
)");

	static ShaderProgram *program2 = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(brush_shader->getCode(), GL_FRAGMENT_SHADER)
			.link();

	program2->bind();
	int id = glGetUniformLocation(program2->getId(),"mouse");
	glUniform2f(id,pos.x,pos.y);
	id = glGetUniformLocation(program2->getId(),"mousePrev");
	glUniform2f(id,prev.x,prev.y);
	id = glGetUniformLocation(program2->getId(),"brush_size");
	glUniform1f(id,brush_size);
	id = glGetUniformLocation(program2->getId(),"brush_flow");
	glUniform1f(id,flow);

	//TODO setup brush calc, move to method?
	{
		auto v = p->getCoords();

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

		int id = glGetUniformLocation(program2->getId(),"rotation");
		glUniformMatrix4fv(id,1,GL_FALSE,glm::value_ptr(rotation));

		p->setCanvasUniforms(program2);
	}

	p->apply(program2,p->get_terrain());
	p->get_terrain()->swap(p->get_scratch2());

	//Transfer to terrain
	ShaderProgram *program3 = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(R"(
#version 430
in vec2 st;
uniform sampler2D scratch1;
uniform sampler2D scratch2;
out float fc;

uniform float brush_value=1.0f;
uniform float brush_limit;

void main () {
    fc = texture(scratch1, st).r + brush_value*min(texture(scratch2,st).r,brush_limit);
}
    )", GL_FRAGMENT_SHADER)
				.link();

	program3->bind();
	id = glGetUniformLocation(program3->getId(),"brush_value");
	glUniform1f(id,value);
	id = glGetUniformLocation(program3->getId(),"brush_limit");
	glUniform1f(id,limit);

	p->apply(program3,p->get_terrain());
}

void BrushWindow::set_hardness(float hardness) {
	float data[brush_tex_size*brush_tex_size];

	auto brush_val = [hardness](float x) {
		double R = 1.0f;
		double phi = x/R;
		double result;
		if (phi<=hardness) result=1; else result=0;
		double c = M_PI*phi/(2*(1-hardness))+M_PI/2*(1-1/(1-hardness));
		if (phi>hardness) result = pow(cos(c),2);
		return (float) result;
	};

	for (int i=0; i<brush_tex_size; i++) {
		float d = i/((float)brush_tex_size-1);
		float width = sqrt(1-pow(d,2));
		float current = -width;
		float step = (2*width)/(brush_tex_size-1);

		float r = sqrt(pow(d,2)+pow(current+i*step,2));
		float current_val = brush_val(r);
		float sum = 0;

		for (int j=0; j<brush_tex_size; j++) {
			current+=step;
			float r = sqrt(pow(d,2)+pow(current,2));
			float new_val = brush_val(r);
			sum+= (current_val+new_val)/2*step;
			data[i*brush_tex_size+j] = sum;
			current_val = new_val;
		}
	}
	brush_tex->uploadData(GL_RED,GL_FLOAT,data);
}