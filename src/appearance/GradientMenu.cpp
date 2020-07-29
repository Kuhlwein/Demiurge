//
// Created by kuhlwein on 7/17/20.
//

#include <vector>
#include <iostream>
#include "GradientMenu.h"
#include "Texture.h"

GradientMenu::GradientMenu(std::string name,std::vector<int> colors) {
	this->name = name;
	setGradient(colors);
}

void GradientMenu::update() {
	ImGui::Text(name.c_str());
	if(ImGui::GradientButton(&gradient))
		ImGui::OpenPopup(name.c_str());
	if (ImGui::BeginPopupModal(name.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::GradientEditor(&gradient, draggingMark, selectedMark);
		ImGui::SameLine();
		if(ImGui::DragFloat("",&(selectedMark->position),0.01f,0.0f,1.0f,"Position: %.3f")) gradient.sortMarks();
		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();
		if (ImGui::Button("OK", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void GradientMenu::setGradient(std::vector<int> colors) {
	gradient.getMarks().clear();
	for (long unsigned int i=0; i<colors.size()/3; i++) {
		gradient.addMark(float(i)/(colors.size()/3-1),ImColor(colors[i*3],colors[i*3+1],colors[i*3+2],255));
	}
}

void GradientMenu::toTexture(Texture *texture) {
	auto data = new unsigned char[100 * 4];
	for (int i=0; i<100; i++) {
		float c[4];
		gradient.computeColorAt(float(i)/100.0f,c);
		data[4*i]=int(c[0]*255);
		data[4*i+1]=char(c[1]*255);
		data[4*i+2]=char(c[2]*255);
		data[4*i+3]=char(c[3]*255);
	}
	texture->uploadData(GL_RGBA, GL_UNSIGNED_BYTE, data);
	delete data;
}


