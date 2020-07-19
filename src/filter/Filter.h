//
// Created by kuhlwein on 7/18/20.
//

#ifndef DEMIURGE_FILTER_H
#define DEMIURGE_FILTER_H

#include <glm/glm.hpp>
#include <ShaderProgram.h>
#include <Shader.h>

class Project;

class Filter {
public:
	Filter() = default;
	virtual void run(Project* p) = 0;
	virtual void finalize(Project* p) = 0;
	virtual bool isfinished() = 0;
	virtual Shader* getShader() = 0;
	//Preview??
	//shader view

};

class NoneFilter : public Filter {
	void run(Project* p) override {}
	void finalize(Project* p) override {}
	bool isfinished() override {return true;};
	virtual Shader* getShader() override {return Shader::builder().create("","");}

public:
	NoneFilter() = default;
};

class FreeSelectFilter : public Filter {
public:
	FreeSelectFilter(Project* p);
	void run(Project* p) override;
	void finalize(Project* p) override;
	bool isfinished() override;
	Shader* getShader() override;

private:
	glm::vec2 first_mousepos;
	ShaderProgram* program;
};


#endif //DEMIURGE_FILTER_H
