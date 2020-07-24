//
// Created by kuhlwein on 7/24/20.
//

#ifndef DEMIURGE_FLATGEOMETRY_H
#define DEMIURGE_FLATGEOMETRY_H


#include "Geometry.h"

class Shader;
class ShaderProgram;
class Project;

class PseudoFlatGeometry : public Geometry {
public:
	PseudoFlatGeometry(Project* p, std::string brush_code, std::string distance_code, std::string offset_code, std::string free_select_code);
	void setup_brush_calc(ShaderProgram *program, glm::vec2 pos, glm::vec2 prev) override;
	void setup_triangle(ShaderProgram* program, glm::vec2 a, glm::vec2 b, glm::vec2 c);
};

class FlatGeometry : public PseudoFlatGeometry {
public:
	FlatGeometry(Project* p);
};

class WrapXGeometry : public PseudoFlatGeometry {
public:
	WrapXGeometry(Project* p);
};

class WrapYGeometry : public PseudoFlatGeometry {
public:
	WrapYGeometry(Project* p);
};

class WrapXYGeometry : public PseudoFlatGeometry {
public:
	WrapXYGeometry(Project* p);
};


#endif //DEMIURGE_FLATGEOMETRY_H
