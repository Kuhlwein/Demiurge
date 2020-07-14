//
// Created by kuhlwein on 7/14/20.
//

#ifndef DEMIURGE_APPEARANCEWINDOW_H
#define DEMIURGE_APPEARANCEWINDOW_H

#include <Menu.h>

class Project;
class Appearance;

class AppearanceWindow : public Window {
public:
	AppearanceWindow(std::string title);
	bool update(Project* p) override;
private:
	std::vector<Appearance*> appearances;
};



#endif //DEMIURGE_APPEARANCEWINDOW_H
