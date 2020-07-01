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

class GeometryShader {
public:
	GeometryShader(Project* p);
	virtual Shader* get_shader();
	virtual std::function<void(glm::vec2 pos, glm::vec2 prev,ShaderProgram* program)> get_setup();
protected:
	Project* p;
};

class SphericalShader : public GeometryShader {
public:
	SphericalShader(Project* p);
	Shader* get_shader() override;
	std::function<void(glm::vec2 pos, glm::vec2 prev, ShaderProgram* program)> get_setup() override;
};

class NoneShader : public GeometryShader {
public:
	NoneShader(Project* p);
	Shader* get_shader() override;
	std::function<void(glm::vec2 pos, glm::vec2 prev, ShaderProgram* program)> get_setup() override;
};

class XShader : public NoneShader {
public:
	XShader(Project* p) : NoneShader(p) {}
	Shader* get_shader() override;
};

class YShader : public NoneShader {
public:
	YShader(Project* p) : NoneShader(p) {}
	Shader* get_shader() override;
};

class XYShader : public NoneShader {
public:
	XYShader(Project* p) : NoneShader(p) {}
	Shader* get_shader() override;
};

#endif //DEMIURGE_EDIT_H
