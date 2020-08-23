//
// Created by kuhlwein on 4/11/20.
//

#include "Project.h"
#include <imgui/imgui.h>
#include <algorithm>
#include "selection.h"
#include "AllSelect.h"
#include "InverseSelection.h"
#include "FreeSelection.h"
#include "BlurSelection.h"
#include "BorderSelection.h"
#include "GrowShringSelection.h"
#include "HeightSelection.h"


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
 * Border
 * Blur
 */

std::vector<Menu*> selection::get_selection_menu() {
	std::vector<Menu*> selection_menu = {};

	selection_menu.push_back(new AllSelect());
	selection_menu.push_back(new InverseSelection());
	selection_menu.push_back(new FreeSelection());
	selection_menu.push_back(new GrowShrinkMenu());
	selection_menu.push_back(new BorderMenu());
	selection_menu.push_back(new BlurSelection());
		auto fromterrain = new SubMenu("From...");
		fromterrain->addMenu(new HeightSelection());
		//direction
		//slope
		//layer
		selection_menu.push_back(fromterrain);
	return selection_menu;
}


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



//
//bool selection::by_height(Project* p) {
//	static float low = -1;
//	static float high = 1;
//	bool updated = ImGui::DragFloat("Low", &low, 0.01f, -1.0f, high, "%.2f", 1.0f);
//	updated = updated | ImGui::DragFloat("High", &high, 0.01f, low, 1.0f, "%.2f", 1.0f);
//
//	auto f = [](Project* p) {
//		Shader* fragment_set = Shader::builder()
//				.include(fragmentBase)
//				.create(R"(
//uniform float low=0;
//uniform float high=0;
//)",R"(
//float val = texture(img,st).r;
//if (val<high && val>low) {
//fc = 1.0f;
//} else {
//fc = 0.0f;
//}
//)");
//
//		ShaderProgram *program = ShaderProgram::builder()
//				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
//				.addShader(fragment_set->getCode(), GL_FRAGMENT_SHADER)
//				.link();
//		program->bind();
//		int id = glGetUniformLocation(program->getId(),"low");
//		glUniform1f(id,low);
//		id = glGetUniformLocation(program->getId(),"high");
//		glUniform1f(id,high);
//		p->apply(program,p->get_selection());
//		delete fragment_set; delete program;
//		return 2.0;
//	};
//
//	if (updated) p->preview(f, [](Project *p) { return p->get_selection(); });
//
//	if (ImGui::Button("Apply")) {
//		p->stop_preview();
//		p->add_filter(f, [](Project *p) { return p->get_selection(); });
//		return true;
//	}
//	return false;
//};
//
