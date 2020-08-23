//
// Created by kuhlwein on 7/18/20.
//

#ifndef DEMIURGE_FREESELECTION_H
#define DEMIURGE_FREESELECTION_H



#include <glm/glm.hpp>
#include <Menu.h>
#include <filter/Filter.h>

class FreeSelection : public Modal {
public:
	FreeSelection();
	bool update_self(Project* p);
private:
	std::shared_ptr<Filter> filter;
};

class FreeSelectFilter : public BackupFilter {
public:
	FreeSelectFilter(Project *p, Shader* mode);
	~FreeSelectFilter();
	void run(Project* p) override;
	Shader* getShader() override;
	bool isFinished() override;
private:
	glm::vec2 first_mousepos;
	ShaderProgram* program;
	Shader* mode;
	bool finished = false;
};


#endif //DEMIURGE_FREESELECTION_H
