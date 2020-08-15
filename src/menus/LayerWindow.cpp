//
// Created by kuhlwein on 8/15/20.
//

#include "Project.h"
#include <imgui/imgui.h>
#include <UndoHistory.h>
#include <iostream>
#include "LayerWindow.h"

LayerWindow::LayerWindow() : Window("Layers", [this](Project* project) {


	int selected = project->get_current_layer();
	static char path[128] = "";
	static int lastselected = -1;
	auto current_layer = project->get_layer(selected);

	if (selected != lastselected) for (int i=0; i<128; i++) {
		if (i<current_layer->getName().size()) path[i] = current_layer->getName()[i];
		else path[i] = '\0';
	}

	if (ImGui::InputText("Layer name",path,IM_ARRAYSIZE(path))) {
		current_layer->setName(std::string(path));
	}

	if (ImGui::ListBoxHeader("List")) {
		for (auto l : project->get_layers()) {
			auto layer = l.second;
			std::string layer_name =layer->getName();
			if (ImGui::Selectable((layer_name + "##" + std::to_string(layer->id)).c_str(), layer->id == selected)) {
				auto u = [selected](Project *p) {
					p->set_layer(selected);
				};
				auto r = [i = l.first](Project *p) {
					p->set_layer(i);
				};
				auto hist = new ReversibleHistory(r, u);
				hist->redo(project);
				project->add_history(hist);
			}
		}
		ImGui::ListBoxFooter();
	}


	if (ImGui::Button("Create new layer")) {
		auto u = [](Project* p){
			std::pair<int,Layer*> maxl = {-1,nullptr};
			for(auto l : p->get_layers()) if(l.first>maxl.first) maxl=l;
			p->remove_layer(maxl.first);
			std::cout << "deleting " << maxl.first << "\n";
			delete maxl.second;
		};
		auto r = [](Project* p) {
			p->add_layer(new Layer(p->getWidth(),p->getHeight()));
		};
		auto hist = new ReversibleHistory(r,u);
		hist->redo(project);
		project->add_history(hist);
	}

	ImGui::SameLine();
	if (ImGui::Button("Remove") && project->get_layers().size()>1) {
		auto hist = new deleteLayerHistory(selected);
		hist->redo(project);
		project->add_history(hist);
	}

	return false;
}) {

}

int Layer::id_counter=0;

Layer::Layer(int w, int h) : id(id_counter) {
	id_counter++;
	name = "New Layer";

	texture = new Texture(w,h,GL_R32F,"img");

}

Texture *Layer::getTexture() {
	return texture;
}

std::string Layer::getName() {
	return name;
}

void Layer::setName(std::string s) {
	name = s;
}

Layer::~Layer() {
	std::cout << "layer deleted: " << id << "\n";
	id_counter--;
	delete texture;
}
