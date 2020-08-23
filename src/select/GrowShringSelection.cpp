//
// Created by kuhlwein on 8/23/20.
//
#include "Project.h"
#include <filter/Morphological.h>
#include "GrowShringSelection.h"


GrowShrinkMenu::GrowShrinkMenu() : FilterModal("Grow/Shrink") {

}

void GrowShrinkMenu::update_self(Project *p) {
	ImGuiIO io = ImGui::GetIO();
	const char* items[] = { "Shrink","Grow"};
	ImGui::Combo("Operation",&current, items,IM_ARRAYSIZE(items));
	ImGui::DragFloat("Radius", &radius, 0.01f, 0.1f, 100.0f, "%.2f", 1.0f);
}

std::shared_ptr<BackupFilter> GrowShrinkMenu::makeFilter(Project *p) {
	auto morph = std::make_unique<Morphological>(p, radius, p->get_selection(),(current==0) ? "min" : "max");
	return std::make_shared<ProgressFilter>(p,[](Project* p){return p->get_selection();},std::move(morph));
}