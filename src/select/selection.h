//
// Created by kuhlwein on 4/11/20.
//

#ifndef DEMIURGE_SELECTION_H
#define DEMIURGE_SELECTION_H

class Project;
class Shader;

namespace selection {
	Shader* selection_mode();
	bool by_height(Project* p);
	bool invert(Project* p);
	bool blur(Project* p);
}

#endif //DEMIURGE_SELECTION_H
