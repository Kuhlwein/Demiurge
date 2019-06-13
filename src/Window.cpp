//
// Created by kuhlwein on 6/11/19.
//

#include <iostream>
#include <imgui/imgui.h>
#include "Window.h"

Window::Window(std::string title, void(*fun)(Project*)) {
    this->title = title;
    this->fun = fun;
}

void Window::open() {
    isOpen = true;
}

void Window::update(Project* project) {
    ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);
    if (isOpen) {
        ImGui::Begin(title.c_str(), &isOpen);
        fun(project);
        ImGui::End();
    }
}

void Window::menu() {
    if(ImGui::MenuItem(title.c_str(), nullptr,false,true)) open();
}

void testnamespace::test(Project* project) {
    ImGui::Bullet();
    static float a;
    ImGui::DragFloat("float",&a);
    ImGui::Text("hejhejhehjh");
    static ImVec2 vec(0,0); if(ImGui::Button("label",vec)) {
        std::cout<<"hej\n";
    }
    ImGui::Text("hej");
}