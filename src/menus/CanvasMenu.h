//
// Created by kuhlwein on 7/10/20.
//

#ifndef DEMIURGE_CANVASMENU_H
#define DEMIURGE_CANVASMENU_H


#include <Menu.h>
#include <projections/Canvas.h>

class CanvasMenu : public Modal {
	//bool update(Project* project) override;
public:
	CanvasMenu(std::string title, AbstractCanvas *canvas);
	bool test(Project* p);
};


#endif //DEMIURGE_CANVASMENU_H
