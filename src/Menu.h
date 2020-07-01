//
// Created by kuhlwein on 6/11/19.
//

#ifndef DEMIURGE_MENU_H
#define DEMIURGE_MENU_H


#include <string>
#include <vector>
#include <functional>

class Project;

class Menu {
public:
    Menu(std::string title, std::function<bool(Project *p)> fun);
    virtual void menu();
    virtual void open();
    virtual bool update(Project* project);
protected:
	std::string title;
	bool isOpen;
	std::function<bool(Project *p)> fun;
};

class Window : public Menu {
	bool update(Project* project) override;
public:
	Window(std::string title, std::function<bool(Project *p)> fun);
};

class Modal : public Menu {
	bool update(Project* project) override;
public:
	Modal(std::string title, std::function<bool(Project *p)> fun);

};

class SubMenu : public Menu {
	void menu() override;
	bool update(Project* project) override;
public:
	explicit SubMenu(std::string title);
	void addMenu(Menu* m);
	void addSeparator();
private :
	std::vector<Menu*> menus;
};

class SeparatorMenu : public Menu {
public:
	SeparatorMenu();
	void menu() override;
};

namespace testnamespace {
    bool file_new(Project* project);
	bool file_load(Project *project);
	bool layers(Project *project);
	bool brush(Project *project);
}




#endif //DEMIURGE_MENU_H
