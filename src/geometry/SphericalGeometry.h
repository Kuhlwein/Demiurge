//
// Created by kuhlwein on 7/24/20.
//

#ifndef DEMIURGE_SPHERICALGEOMETRY_H
#define DEMIURGE_SPHERICALGEOMETRY_H


#include "Geometry.h"

class SphericalGeometry : public Geometry {
public:
	SphericalGeometry(Project* p);
	void setup_brush_calc(ShaderProgram *program, glm::vec2 pos, glm::vec2 prev) override;
	void setup_triangle(ShaderProgram* program, glm::vec2 a, glm::vec2 b, glm::vec2 c);
};

#endif //DEMIURGE_SPHERICALGEOMETRY_H
