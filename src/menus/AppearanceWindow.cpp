//
// Created by kuhlwein on 7/14/20.
//

#include <imgui/imgui.h>
#include <Shader.h>
#include <appearance/Appearance.h>
#include <appearance/ElevationMap.h>
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
	if(ImGui::Button("Add")) {
		appearances.insert(appearances.begin(), new ElevationMap());
	}
	ImGui::SameLine();
	if(ImGui::Button("Remove")) {

	}

	for (int n = 0; n < appearances.size(); n++)
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

			}
			ImGui::EndDragDropTarget();
		}
		ImGui::PopID();
	}
	for (Appearance* a : appearances) a->update(p);

			return false;
		}) {
	appearances = {new ElevationMap()};
}

bool AppearanceWindow::update(Project *p) {
	bool r = Window::update(p);
	for(Appearance* a : appearances) a->prepare(p);
	return r;
}

