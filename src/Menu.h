//
// Created by kuhlwein on 5/3/19.
//

#ifndef DEMIURGE_MENU_H
#define DEMIURGE_MENU_H

#include <map>
#include "imgui/imgui_color_gradient.h"

class Project;

class Menu {
public:
    Menu(Project* project);
    ~Menu();
    void render();
    void update();
    void open(void (*)(bool*));
private:
    void hej(bool*);
    Project* project;
    void (*current)(bool*);
    bool isOpen;
    ImGradient gradient;
};


#endif //DEMIURGE_MENU_H
