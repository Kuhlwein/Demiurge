//
// Created by kuhlwein on 7/18/20.
//

#ifndef DEMIURGE_FREESELECTMODAL_H
#define DEMIURGE_FREESELECTMODAL_H


#include <Menu.h>
#include <glm/glm.hpp>

class FreeSelectModal : public Modal {
public:
	FreeSelectModal();
	bool update_self(Project* p);

private:
	bool first_mouseclick = true;
	glm::vec2 first_mousepos;
	ShaderProgram* program;

};


#endif //DEMIURGE_FREESELECTMODAL_H
