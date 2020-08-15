//
// Created by kuhlwein on 8/15/20.
//

#include "Project.h"
#include <imgui/imgui.h>
#include <UndoHistory.h>
#include "LayerWindow.h"

LayerWindow::LayerWindow() : Window("Layers", [this](Project* project) {
	int selected = project->get_current_layer();
	if (ImGui::ListBoxHeader("List")) {
		for (int i=project->get_n_layers()-1; i>=0; i--) {
			std::string layer_name = (project->get_layer(i)).first;
			if (ImGui::Selectable((layer_name + "##" + std::to_string(i)).c_str(), i == selected)) {
				int prev_layer = project->get_current_layer();
				auto u = [prev_layer](Project *p) {
					p->set_layer(prev_layer);
				};
				auto r = [i](Project *p) {
					p->set_layer(i);
				};
				auto hist = new ReversibleHistory(r, u);
				hist->redo(project);
				project->add_history(hist);
			}
		}
		ImGui::ListBoxFooter();
	}

	static char path[128] = "New layer";
	std::string test;
	bool exec = ImGui::InputText("Layer name",path,IM_ARRAYSIZE(path),ImGuiInputTextFlags_EnterReturnsTrue);
	exec = exec | ImGui::Button("Create");
	std::string name = std::string(path);
	if (exec) {

		auto u = [selected](Project* p){
			p->remove_layer(p->get_n_layers()-1);
			p->set_layer(selected);
		};
		auto r = [name](Project* p) {
			p->add_layer({name, new Texture(p->getWidth(), p->getHeight(), GL_R32F, "img")});
		};
		auto hist = new ReversibleHistory(r,u);
		hist->redo(project);
		project->add_history(hist);
	}
	ImGui::SameLine();
	if (ImGui::Button("Remove") && selected != 0) {
		auto layer = project->get_layer(selected);
		auto u = [layer,selected](Project* p){
			p->add_layer(layer,selected);
			p->set_layer(selected);
		};
		auto r = [selected](Project* p) {
			p->remove_layer(selected);
		};
		auto hist = new ReversibleHistory(r,u);
		hist->redo(project);
		project->add_history(hist);
	}

	return false;
}) {

}

int Layer::id_counter=0;

Layer::Layer(int w, int h) {
	id = id_counter;
	id_counter++;

	texture = new Texture(w,h,GL_R32F,"img");

}
