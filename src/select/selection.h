//
// Created by kuhlwein on 4/11/20.
//

#ifndef DEMIURGE_SELECTION_H
#define DEMIURGE_SELECTION_H

#include <filter/FilterModal.h>

class Project;
class Shader;

namespace selection {
	Shader* selection_mode();
	std::vector<Menu*> get_selection_menu();
	bool by_height(Project* p);
}



#endif //DEMIURGE_SELECTION_H
