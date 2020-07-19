//
// Created by kuhlwein on 7/18/20.
//

#include <Shader.h>
#include "FreeSelectModal.h"
#include "Project.h"

FreeSelectModal::FreeSelectModal() : Modal("Free select", [this](Project* p) {
	return this->update_self(p);
}) {

}

bool FreeSelectModal::update_self(Project *p) {
	ImGuiIO io = ImGui::GetIO();



	if(ImGui::Button("Ok")) {
		return true;
	}

	bool a = ImGui::IsAnyWindowHovered() || ImGui::IsAnyItemHovered();
	if(!a && io.MouseDown[0] && io.MouseDownDuration[0]==0) {
		p->NEW_dispatchFilter(new FreeSelectFilter(p));
	}


	return false;
}

