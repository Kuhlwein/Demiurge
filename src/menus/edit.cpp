//
// Created by kuhlwein on 4/16/20.
//
#include <iostream>
#include "Project.h"
#include "edit.h"


bool edit::undo(Project* p) {
	p->undo();
	return true;
}

bool edit::redo(Project* p) {
	p->redo();
	return true;
}

bool edit::preferences(Project* p) {
	static bool first = true;
	static float begin, end, begin2, end2;

	if (first) {
		first = false;
		auto v = p->getCoords();
		begin = -v[1]/M_PI*180.0f;
		end = -v[0]/M_PI*180.0f;
		begin2 = v[2]/M_PI*180.0f;
		end2 = v[3]/M_PI*180.0f;
	}

	bool updated = false;
	updated |= ImGui::DragFloatRange2("Lattitude", &begin, &end, 0.25f, -90.0f, 90.0f, "Min: %.1f", "Max: %.1f");
	updated |= ImGui::DragFloatRange2("Longitude", &begin2, &end2, 0.25f, -180.0f, 180.0f, "Min: %.1f", "Max: %.1f");
	if (updated) p->setCoords({-end,-begin,begin2,end2});

	static float circumference = 42000;
	if (ImGui::DragFloat("Planet circumference",&circumference,10,1,MAXFLOAT,"%.1f m")) {
		p->circumference = circumference;
	}

	if (ImGui::Button("Apply")) {
		first = true;
		return true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel")) {
		first = true;
		return true;
	}
	return false;
}
