//
// Created by kuhlwein on 4/11/20.
//

#include <iostream>
#include <imgui/imgui.h>
#include <algorithm>
#include "Menu.h"
#include "Project.h"
#include "selection.h"


/*
 * All
 * Inverse
 * Free
 * TODO
 * From terrain:
 * 	height
 * 	direction
 * 	slope
 * Expand
 * Contract
 */

Shader* selection::selection_mode() {
	ImGuiIO io = ImGui::GetIO();

	static Shader* replace = Shader::builder().create(R"(
float selection_mode(float old, float new) {
	return new;
}
)","");
	static Shader* add = Shader::builder().create(R"(
float selection_mode(float old, float new) {
	return min(old+new,1);
}
)","");
	static Shader* subtract = Shader::builder().create(R"(
float selection_mode(float old, float new) {
	return max(old-new,0);
}
)","");
	static Shader* intersect = Shader::builder().create(R"(
float selection_mode(float old, float new) {
	return old*new;
}
)","");

	static int current = 0;
	static bool pressed = false;

	if (io.KeyShift || io.KeyCtrl) pressed = true;
	if (io.KeyShift && io.KeyCtrl) current = 3;
	else if (io.KeyShift) current = 1;
	else if (io.KeyCtrl) current = 2;
	else if (pressed) {
		current = 0;
		pressed = false;
	}

	const char* items[] = { "Replace","Add", "Subtract","Intersect"};
	ImGui::Combo("Preset",&current,items,IM_ARRAYSIZE(items));
	switch (current) {
		case 0:
			return replace;
		case 1:
			return add;
		case 2:
			return subtract;
		default:
			return intersect;
	}
}

bool selection::by_height(Project* p) {
	static float low = -1;
	static float high = 1;
	bool updated = ImGui::DragFloat("Low", &low, 0.01f, -1.0f, high, "%.2f", 1.0f);
	updated = updated | ImGui::DragFloat("High", &high, 0.01f, low, 1.0f, "%.2f", 1.0f);

	auto f = [](Project* p) {
		Shader* fragment_set = Shader::builder()
				.include(fragmentBase)
				.create(R"(
uniform float low=0;
uniform float high=0;
)",R"(
float val = texture(img,st).r;
if (val<high && val>low) {
fc = 1.0f;
} else {
fc = 0.0f;
}
)");

		ShaderProgram *program = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(fragment_set->getCode(), GL_FRAGMENT_SHADER)
				.link();
		program->bind();
		int id = glGetUniformLocation(program->getId(),"low");
		glUniform1f(id,low);
		id = glGetUniformLocation(program->getId(),"high");
		glUniform1f(id,high);
		p->apply(program,p->get_selection());
		delete fragment_set; delete program;
		return 2.0;
	};

	if (updated) p->preview(f, [](Project *p) { return p->get_selection(); });

	if (ImGui::Button("Apply")) {
		p->stop_preview();
		p->add_filter(f, [](Project *p) { return p->get_selection(); });
		return true;
	}
	return false;
};

















bool selection::blur(Project *p) {
	static float radius = 1;
	static float first = true;
	auto f = [](Project* p) mutable {
		auto t1 = new Texture(p->getWidth(),p->getHeight(),GL_R32F,"img",GL_LINEAR);
		auto t2 = new Texture(p->getWidth(),p->getHeight(),GL_R32F,"img",GL_LINEAR);

		ShaderProgram *program = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(copy_img->getCode(), GL_FRAGMENT_SHADER)
				.link();
		p->apply(program,t1,{{p->get_terrain(),"to_be_copied"}});

		//TODO FIX
		Shader* fragment_set = Shader::builder()
				.include(fragmentBase)
				//.include(p->getGeometryShader()->get_shader())
				.include(blur13)
				.create("uniform vec2 direction;",R"(
fc = blur13(img,st,direction);
)");
		ShaderProgram *program2 = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(fragment_set->getCode(), GL_FRAGMENT_SHADER)
				.link();


		int id = glGetUniformLocation(program2->getId(),"direction");

		//todo: needs work on consistensy at  low R
		float R = radius*radius/2;
		//std::cout << "R " << R << "\n";
		std::vector<float> rlist = {};
		float i = 0.5f;
		float incrementer = 0.5f;
		if (R<3) {
			float k = 1/sqrt(55/R);
			incrementer = k;
			i= k;
		}
		while (R>=i*i) {
			R-=i*i;
			rlist.push_back(i);
			i+=incrementer;
		}
		if (R>0.0f) rlist.push_back(sqrt(R));
		std::sort(rlist.begin(),rlist.end());
		//std::cout << "begin: ";
		//for (auto a : rlist) std::cout << a << " hej ";


		for (float a : rlist) {
			glUniform2f(id,0,a);
			p->apply(program2,t2,{{t1,"img"}});
			glUniform2f(id,a,0);
			p->apply(program2,t1,{{t2,"img"}});
		}

		p->apply(program,p->get_terrain(),{{t1,"to_be_copied"}});

		delete t1;
		delete t2;
		delete program;
		delete program2;

		return 2.0;
	};


	if ((ImGui::DragFloat("Radius", &radius, 0.01f, 0.1f, 100.0f, "%.2f", 1.0f) || first) && radius < 10) {
		p->preview(f, [](Project *p) { return p->get_terrain(); });
		first = false;
	} else if(radius >=10) ImGui::Text("Live preview disabled for radius > 10");


	if (ImGui::Button("Apply")) {
		p->stop_preview();
		p->add_filter(f, [](Project *p) { return p->get_terrain(); });
		first = true;
		return true;
	}


	return false;
}
