//
// Created by kuhlwein on 6/11/19.
//

#ifndef DEMIURGE_WINDOW_H
#define DEMIURGE_WINDOW_H


#include <string>
class Project;

class Window {
public:
    Window(std::string title, void (*fun)(Project*));

    void menu();
    void open();

    void update(Project* project);

private:
    std::string title;
    bool isOpen;
    void (* fun)(Project*);

};

namespace testnamespace {
    void test(Project* project);
}

#endif //DEMIURGE_WINDOW_H
