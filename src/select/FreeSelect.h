//
// Created by kuhlwein on 7/18/20.
//

#ifndef DEMIURGE_FREESELECT_H
#define DEMIURGE_FREESELECT_H



#include <glm/glm.hpp>
#include <Menu.h>
#include <filter/Filter.h>

class FreeSelect : public Modal {
public:
	FreeSelect();
	bool update_self(Project* p);
};

class FreeSelectFilter : public BackupFilter {
public:
	FreeSelectFilter(Project *p, Shader* mode);
	~FreeSelectFilter();
	void run() override;
	void finalize() override;
	Shader* getShader() override;
private:
	glm::vec2 first_mousepos;
	ShaderProgram* program;
	Shader* mode;
};


#endif //DEMIURGE_FREESELECT_H
