//
// Created by kuhlwein on 7/14/20.
//

#include "Project.h"
#include <imgui/imgui.h>
#include <Shader.h>
#include <appearance/Appearance.h>
#include <appearance/ElevationMap.h>
#include <appearance/Graticules.h>
#include <iostream>
#include "AppearanceWindow.h"


// Prepare
// Unprepare

AppearanceWindow::AppearanceWindow(std::string title) : Window(title, [this](Project* p) {
	static char str0[128] = "Hello, world!";
	ImGui::InputText("Name", str0, IM_ARRAYSIZE(str0));

	ImGui::Button("New");
	ImGui::SameLine();
	ImGui::Button("From template");

	ImGui::Separator();
	if(ImGui::Button("Add")) ImGui::OpenPopup("my_add_popup");
	if (ImGui::BeginPopup("my_add_popup"))
	{
		if (ImGui::Selectable("Graticules")) add(new Graticules(),p);
		if (ImGui::Selectable("Elevation map")) add(new ElevationMap(),p);
		ImGui::EndPopup();
	}

	ImGui::SameLine();
	if(ImGui::Button("Remove") && appearances.size()>1) ImGui::OpenPopup("my_remove_popup");
	if (ImGui::BeginPopup("my_remove_popup"))
	{
		for (int i = appearances.size()-1; i>=0; i--)
			if (ImGui::Selectable(appearances[i]->getTitle().c_str()))
				appearances.erase(appearances.begin()+i);
		ImGui::EndPopup();
	}


	for (int n = appearances.size()-1; n>=0; n--)
	{
		ImGui::PushID(n);

		if(ImGui::Button(appearances[n]->getTitle().c_str(), ImVec2(250, 20))) {
			appearances[n]->open();
		}

		// Our buttons are both drag sources and drag targets here!
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			ImGui::SetDragDropPayload("DND_DEMO_CELL", &n, sizeof(int));    // Set payload to carry the index of our item (could be anything)
			// Display preview (could be anything, e.g. when dragging an image we could decide to display the filename and a small preview of the image, etc.)
			{ ImGui::Text("Swap %s", appearances[n]->getTitle().c_str()); }
			ImGui::EndDragDropSource();
		}
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_DEMO_CELL"))
			{
				IM_ASSERT(payload->DataSize == sizeof(int));
				int payload_n = *(const int*)payload->Data;

				const auto tmp = appearances[n];
				appearances[n] = appearances[payload_n];
				appearances[payload_n] = tmp;
				setShader(p);

			}
			ImGui::EndDragDropTarget();
		}
		ImGui::PopID();
	}

	for (Appearance* a : appearances) {
		if(a->update(p)) {
			a->prepare(p);
			std::cout << "test\n";
		}
	}

	return false;
		}) {
	appearances = {new ElevationMap()};
}

bool AppearanceWindow::update(Project *p) {
	bool r = Window::update(p);
	//for(Appearance* a : appearances) a->prepare(p);
	return r;
}

void AppearanceWindow::setShader(Project* p) {
	auto shaderbuilder = Shader::builder();
	for (Appearance* a : appearances) {
		shaderbuilder.include(a->getShader());
	}
	auto shader = shaderbuilder.create();
	p->set_terrain_shader(shader);
	for(Appearance* a : appearances) {
		std::cout << "prep " << a->getTitle() << "\n";
		a->prepare(p);
	}
}

void AppearanceWindow::add(Appearance *a, Project* p) {
	appearances.push_back(a);
	setShader(p);
}

