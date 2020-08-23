//
// Created by kuhlwein on 8/23/20.
//

#include "Project.h"
#include <filter/Morphological.h>
#include "BorderSelection.h"

void BorderMenu::update_self(Project *p) {
	ImGuiIO io = ImGui::GetIO();
	ImGui::DragFloat("Width", &radius, 0.01f, 0.1f, 100.0f, "%.2f", 1.0f);
}

std::shared_ptr<BackupFilter> BorderMenu::makeFilter(Project *p) {
	auto morph = std::make_unique<MorphologicalGradient>(p, radius, p->get_selection());
	return std::make_shared<ProgressFilter>(p,[](Project* p){return p->get_selection();},std::move(morph));
}

BorderMenu::BorderMenu() : FilterModal("Border") {

}