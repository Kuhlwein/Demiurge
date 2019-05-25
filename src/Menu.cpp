//
// Created by kuhlwein on 5/3/19.
//

#include <imgui/imgui.h>
#include <iostream>
#include "Menu.h"


Menu::Menu(Project *project) {
    this->project = project;
    current = [](bool* a) {};
}

Menu::~Menu() {

}

void Menu::hej(bool* isOpen) {
    ImGui::SetNextWindowSize(ImVec2(350, 560), ImGuiCond_FirstUseEver);
    if (*isOpen && ImGui::Begin("MORTEN KUHLWEIN", isOpen))
    {
        if(ImGui::GradientButton(&gradient))
        {
            //set show editor flag to true/false
        }

        static ImGradientMark* draggingMark = nullptr;
        static ImGradientMark* selectedMark = nullptr;

        bool updated = ImGui::GradientEditor(&gradient, draggingMark, selectedMark);
        ImGui::End();
    }
}

void Menu::update() {
    if (ImGui::BeginMenu("File",true)) {
        //if(ImGui::MenuItem("New...", nullptr,false,true)) open(hej);
        ImGui::EndMenu();
    }

    current(&isOpen);
    hej(&isOpen);
}

void Menu::open(void (*fun)(bool*)) {
    isOpen = true;
    current = fun;
}





