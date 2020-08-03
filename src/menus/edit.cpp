//
// Created by kuhlwein on 4/16/20.
//
#include <iostream>
#include <geometry/FlatGeometry.h>
#include <geometry/SphericalGeometry.h>
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
		begin = v[0];
		end = v[1];
		begin2 = v[2];
		end2 = v[3];
	}

	static int current = 4;
	const char* items[] = {"Flat","Wrap x-axis", "Wrap y-axis","Toroidal","Spherical"};
	if(ImGui::Combo("Canvas geometry",&current,items,IM_ARRAYSIZE(items))) {

	}

	bool updated = false;
	updated |= ImGui::DragFloatRange2("Lattitude", &begin, &end, 0.25f, -90.0f, 90.0f, "Min: %.1f", "Max: %.1f");
	updated |= ImGui::DragFloatRange2("Longitude", &begin2, &end2, 0.25f, -180.0f, 180.0f, "Min: %.1f", "Max: %.1f");
	if (updated) p->setCoords({begin,end,begin2,end2});

	static float circumference = 42000;
	ImGui::DragFloat("Planet circumference",&circumference,10,1,MAXFLOAT,"%.1f m");

	if (ImGui::Button("Apply")) {
		switch (current) {
			//TODO FIX
			case 0: { p->setGeometry(new FlatGeometry(p)); break;}
			case 1: { p->setGeometry(new WrapXGeometry(p)); break;}
			case 2: { p->setGeometry(new WrapYGeometry(p)); break;}
			case 3: { p->setGeometry(new WrapXYGeometry(p)); break;}
			case 4: { p->setGeometry(new SphericalGeometry(p)); break;}
		}
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
