//
// Created by kuhlwein on 7/13/20.
//

#include <imgui/imgui.h>
#include <iostream>
#include "BrushWindow.h"
#include "Project.h"

BrushWindow::BrushWindow(std::string title, Project* p) : Window(title, [](Project* p){return true;}) {
	brush_tex = new Texture(brush_tex_size,brush_tex_size,GL_R32F,"brush_tex",GL_LINEAR);
	p->add_texture(brush_tex);
	set_brush(0.5f);
}

bool BrushWindow::update(Project *p) {
	if (isOpen) {
		ImGui::Begin(title.c_str(), &isOpen,ImGuiWindowFlags_AlwaysAutoResize);
		if (brush_window(p)) isOpen=false;
		ImGui::End();
	}
	brush_stroke(p);
	return isOpen;
}

bool BrushWindow::brush_window(Project *p) {
	if (ImGui::DragFloat("Value",&value,0.01f,0.0f,1.0f,"%.2f",1.0f)) {
	}

	if (ImGui::DragFloat("Size",&(brush_size),1.0f,1.0f,10000.0f,"%.2f",1.0f)) {
	}

	if (ImGui::DragFloat("Hardness",&(hardness),0.01f,0.0f,1.0f,"%.2f",1.0f)) {
		set_brush(hardness);
	}

	ImGui::DragFloat("flow",&flow,0.01f,0.0f,1.0f,"%.2f",1.0f);

	if (limitEnabled) {
		ImGui::DragFloat("##limit",&limit,0.01f,0.0f,1.0f,"%.2f",1.0f);
		ImGui::SameLine();
	}
	ImGui::Checkbox("Limit",&limitEnabled);

	return false;
}

void BrushWindow::brush_stroke(Project *p) {
	//Brushing
	ImGuiIO io = ImGui::GetIO();

	static bool first = true;
	static std::vector<std::pair<glm::vec2,glm::vec2>> brush_strokes;

	glm::vec2 texcoord = p->getMouse();
	glm::vec2 texcoordPrev = p->getMousePrev();

	auto program = p->program;

	int id = glGetUniformLocation(program->getId(),"mouse");
	glUniform2f(id,texcoord.x,texcoord.y);
	id = glGetUniformLocation(program->getId(),"mousePrev");
	glUniform2f(id,texcoordPrev.x,texcoordPrev.y);
	id = glGetUniformLocation(program->getId(),"brush_size");
	glUniform1f(id,brush_size);

	if(io.WantCaptureMouse) return;

	if(io.MouseDown[0] & first) {
		brush_strokes.erase(brush_strokes.begin(),brush_strokes.end());
		initbrush(p);
		first = false;
	} else if (io.MouseDown[0]) {
		brush(p,texcoord,texcoordPrev);
		brush_strokes.push_back({texcoord,texcoordPrev});
	} else if (!first) {
		std::cout << "last\n";
		//project->brush(texcoord,texcoordPrev);
		brush_strokes.push_back({texcoord,texcoordPrev});
		auto test = brush_strokes;

		float hardness = hardness;
		float brush_size2 = brush_size;
		auto r = [this,test,hardness,brush_size2](Project* p) {
			initbrush(p);
			auto data = brush_tex->downloadData();
			set_brush(hardness);
			float oldsize = brush_size;
			brush_size = brush_size2;
			for (int i=0; i<test.size()-1; i++) brush(p,test[i].first,test[i].second);
			brush_size = oldsize;
			brush_tex->uploadData(GL_RED,GL_FLOAT,data);
		};
		auto u = [this,test,hardness,brush_size2](Project* p) {
			initbrush(p);

			auto data = brush_tex->downloadData();
			set_brush(hardness);
			float oldsize = brush_size;
			brush_size = brush_size2;

			for (int i=0; i<test.size()-1; i++) brush(p,test[i].first,test[i].second,true);
			ShaderProgram *program = ShaderProgram::builder()
					.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
					.addShader(fix_0->getCode(), GL_FRAGMENT_SHADER)
					.link();
			p->apply(program,p->get_scratch1());
			p->get_terrain()->swap(p->get_scratch1());

			brush_size = oldsize;
			brush_tex->uploadData(GL_RED,GL_FLOAT,data);
		};

		auto hist = new ReversibleHistory(r,u);
		p->add_history(hist);
		first = true;
	}
}

void BrushWindow::set_brush(float hardness) {
	float* data = new float[brush_tex_size*brush_tex_size];

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

void BrushWindow::brush(Project* p, glm::vec2 pos, glm::vec2 prev, bool flag) {
	//Paint to scratchpad2

	Shader* brush_shader = Shader::builder()
			.include(fragmentBase)
			.include(p->getGeometryShader()->get_shader())
			.create("",R"(
vec2 vstop;
vec2 vstart;
brush_calc(vstart,vstop);
fc = texture(scratch2,st).r + texture(sel,st).r*(texture(brush_tex,vstop).r - texture(brush_tex,vstart).r);
)");

	ShaderProgram *program2 = ShaderProgram::builder()
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

	(p->getGeometryShader()->get_setup())(pos,prev,program2);

	p->apply(program2,p->get_terrain());
	p->get_terrain()->swap(p->get_scratch2());

	//Transfer to terrain
	ShaderProgram *program3;
	if (!flag) {
		program3 = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(R"(
#version 430
in vec2 st;
uniform sampler2D scratch1;
uniform sampler2D scratch2;
out float fc;

void main () {
    fc = texture(scratch1, st).r + min(texture(scratch2,st).r,0.3);
}
    )", GL_FRAGMENT_SHADER)
				.link();
	} else {
		program3 = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(R"(
#version 430
in vec2 st;
uniform sampler2D scratch1;
uniform sampler2D scratch2;
out float fc;

void main () {
    fc = texture(scratch1, st).r - min(texture(scratch2,st).r,0.3);
}
    )", GL_FRAGMENT_SHADER)
				.link();
	}


	p->apply(program3,p->get_terrain());
}

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
