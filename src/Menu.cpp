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




SeparatorMenu::SeparatorMenu() : Menu("",[](Project* p){return false;}) {

}

