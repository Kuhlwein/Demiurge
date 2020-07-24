//
// Created by kuhlwein on 7/24/20.
//

#ifndef DEMIURGE_GEOMETRY_H
#define DEMIURGE_GEOMETRY_H

#include <glm/glm.hpp>
#include <string>

class Shader;
class ShaderProgram;
class Project;

class Geometry {

public:
	Geometry(Project* p) {
		this->p = p;
	}
	Shader* brush_calc() {return brush_shader;};
	virtual void setup_brush_calc(ShaderProgram *program, glm::vec2 pos, glm::vec2 prev) = 0;
	Shader* distance() { return distance_shader;};
	Shader* offset() {return offset_shader;};

protected:
	Project* p;
	Shader* brush_shader;
	Shader* distance_shader;
	Shader* offset_shader;
};

class PseudoFlatGeometry : public Geometry {
public:
	PseudoFlatGeometry(Project* p, std::string brush_code, std::string distance_code, std::string offset_code);
	void setup_brush_calc(ShaderProgram *program, glm::vec2 pos, glm::vec2 prev) override;
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

class SphericalGeometry : public Geometry {
public:
	SphericalGeometry(Project* p);
	void setup_brush_calc(ShaderProgram *program, glm::vec2 pos, glm::vec2 prev) override;
};

#endif //DEMIURGE_GEOMETRY_H
