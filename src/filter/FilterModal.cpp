//
// Created by kuhlwein on 7/28/20.
//

#include "Project.h"
#include <imgui/imgui.h>
#include "FilterModal.h"
#include "BlurMenu.h"

FilterModal::FilterModal(std::string title) : Modal(title, [this](Project* p) {
	return this->update_FilterModal(p);
}) {
	filter = std::make_shared<NoneFilter>();
}

bool FilterModal::update_FilterModal(Project *p) {
	update_self(p);

	filter->run();

	if(ImGui::Button("Preview")) {
		if (previewing) {
			p->undo();
		}
		previewing = true;

		filter = makeFilter(p);
		p->dispatchFilter(filter);

		return false;
	}
	ImGui::SameLine();
	if(ImGui::Button("Apply")) {
		if (!previewing) {
			filter = makeFilter(p);
			p->dispatchFilter(filter);
		}
		previewing = false;
	}
	ImGui::SameLine();
	if(ImGui::Button("Close")) {
		if (previewing) p->undo();
		previewing=false;
		filter = std::make_shared<NoneFilter>();
		return true;
	}
	if (filter->isFinished() && !previewing) {
		filter = std::make_shared<NoneFilter>();
		return true;
	}

	return false;
}
