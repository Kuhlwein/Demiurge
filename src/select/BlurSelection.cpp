//
// Created by kuhlwein on 8/23/20.
//

#include "Project.h"
#include <filter/BlurMenu.h>
#include "BlurSelection.h"

BlurSelection::BlurSelection() : FilterModal("Blur##Selection") {

}

void BlurSelection::update_self(Project *p) {
	ImGuiIO io = ImGui::GetIO();

	ImGui::DragFloat("Radius", &radius, 0.01f, 0.1f, 100.0f, "%.2f", 1.0f);
}

std::shared_ptr<BackupFilter> BlurSelection::makeFilter(Project* p) {
	return std::make_shared<ProgressFilter>(p, [](Project* p){return p->get_selection();},std::move(std::make_unique<Blur>(p, radius, p->get_selection())));
}