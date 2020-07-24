//
// Created by kuhlwein on 4/16/20.
//

#ifndef DEMIURGE_EDIT_H
#define DEMIURGE_EDIT_H

#include <functional>
#include <glm/glm.hpp>
#include "ShaderProgram.h"
#include "Shader.h"

class Project;

namespace edit {
	bool undo(Project* p);
	bool redo(Project* p);
	bool preferences(Project* p);
}



#endif //DEMIURGE_EDIT_H
