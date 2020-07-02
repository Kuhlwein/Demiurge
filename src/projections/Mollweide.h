//
// Created by kuhlwein on 7/2/20.
//

#ifndef DEMIURGE_MOLLWEIDE_H
#define DEMIURGE_MOLLWEIDE_H


#include "Canvas.h"

class Project;

class Mollweide : public AbstractCanvas {
	public:
		Mollweide(Project* project);
		glm::vec2 mousePos(ImVec2 pos) override;
		Shader* projection_shader() override;
};


#endif //DEMIURGE_MOLLWEIDE_H
