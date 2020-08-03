//
// Created by kuhlwein on 8/3/20.
//

#include "Project.h"
#include <menus/CanvasMenu.h>
#include "Projections.h"
#include "Equiretangular.h"
#include "img.h"
#include "Orthographic.h"
#include "Mollweide.h"
#include "Sinusoidal.h"
#include "GoodeHomolosine.h"
#include "EckertIV.h"
#include "Mercator.h"

std::vector<Menu *> projections::get_projection_menu(Project* p) {
	std::vector<Menu*> projections = {};
	//projections.push_back(new Menu("None", [](Project* p) {
	//	p->canvas = new img(p);
	//	p->update_terrain_shader();
	//	return true;
	//}));
	projections.push_back(new CanvasMenu("Equiretangular...",new Equiretangular(p)));
	projections.push_back(new Menu("Orthographic", [](Project* p){
		p->canvas = new Orthographic(p);
		p->update_terrain_shader();
		return true;
	}));

	projections.push_back(new CanvasMenu("Mollweide...",new Mollweide(p)));
	projections.push_back(new CanvasMenu("Sinusoidal...",new Sinusoidal(p)));
	projections.push_back(new CanvasMenu("Goode Homolosine...",new GoodeHomolosine(p)));
	projections.push_back(new CanvasMenu("Eckert IV...",new EckertIV(p)));
	projections.push_back(new CanvasMenu("Mercator...",new Mercator(p)));
	return projections;
}
