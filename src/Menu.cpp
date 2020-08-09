//
// Created by kuhlwein on 6/11/19.
//

#include <iostream>
#include <imgui/imgui.h>
#include "Menu.h"
#include "Project.h"

Menu::Menu(std::string title, std::function<bool(Project *p)> fun) {
    this->title = title;
	this->fun = fun;
	isOpen = false;
}
void Menu::open() {
    isOpen = true;
}
bool Menu::update(Project* project) {
    if (isOpen) {
        if (fun(project)) isOpen=false;
    }
    return isOpen;
}
void Menu::menu() {
    if(ImGui::MenuItem(title.c_str(), nullptr,false,true)) open();
}

Window::Window(std::string title, std::function<bool(Project *p)> fun) : Menu(title, fun) {}
bool Window::update(Project* project) {
	if (isOpen) {
		ImGui::Begin(title.c_str(), &isOpen,ImGuiWindowFlags_AlwaysAutoResize);
		if (fun(project)) isOpen=false;
		ImGui::End();
	}
	return isOpen;
}

Modal::Modal(std::string title, std::function<bool(Project *p)> fun) : Menu(title, fun) {}
bool Modal::update(Project *project) {
	if (isOpen) {
		ImGui::OpenPopup(title.c_str());
		isOpen = false;
	}
	bool closing = true;
	ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg,ImVec4(0.60f, 0.60f, 0.60f, 0.0f));
	if (ImGui::BeginPopupModal(title.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		closing = fun(project);
		if (closing) ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
	ImGui::PopStyleColor(1);
	return !closing;
}

SubMenu::SubMenu(std::string title) : Menu(title, [](Project* p){return false;}) {}
void SubMenu::menu() {
	if (ImGui::BeginMenu(title.c_str())) {
		for (auto m : menus) m->menu();
		ImGui::EndMenu();
	}
}
bool SubMenu::update(Project *project) {
	for (auto m : menus) m->update(project);
	return isOpen;
}
void SubMenu::addMenu(Menu *m) {
	menus.push_back(m);
}
void SubMenu::addSeparator() {
	class SeparatorMenu : public Menu {
	public:
		SeparatorMenu() : Menu("",[](Project* p){return false;}) {

		}
		void menu() override {
			ImGui::Separator();
		}
	};
	auto sep = new SeparatorMenu();
	menus.push_back(sep);
}

void SeparatorMenu::menu() {
	ImGui::Separator();
}

bool testnamespace::file_load(Project* project) {
	static char path[128] = "/home/kuhlwein/Desktop/world.jpg";
    bool exec = ImGui::InputText("path",path,IM_ARRAYSIZE(path),ImGuiInputTextFlags_EnterReturnsTrue);
    exec = exec | ImGui::Button("test button");
	if (exec) {
		project->file_load(path);
		return true;
	}
	return false;
}

bool testnamespace::file_new(Project *project) {
	static int width=100, height=100;
	ImGui::InputInt("width",&width,1,100);
	ImGui::InputInt("height",&height,1,100);

	if (ImGui::Button("test button")) {
		project->file_new(width,height);
		return true;
	}
	return false;
}


bool testnamespace::layers(Project* project) {
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
}


SeparatorMenu::SeparatorMenu() : Menu("",[](Project* p){return false;}) {

}

