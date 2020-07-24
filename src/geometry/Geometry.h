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
	Shader* triangle() {return triangle_shader;};
	virtual void setup_triangle(ShaderProgram* program, glm::vec2 a, glm::vec2 b, glm::vec2 c) = 0;

protected:
	Project* p;
	Shader* brush_shader;
	Shader* distance_shader;
	Shader* offset_shader;
	Shader* triangle_shader;
};


#endif //DEMIURGE_GEOMETRY_H
