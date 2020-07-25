//
// Created by kuhlwein on 7/18/20.
//

#ifndef DEMIURGE_FREESELECTMODAL_H
#define DEMIURGE_FREESELECTMODAL_H



#include <glm/glm.hpp>
#include <Menu.h>
#include <filter/Filter.h>

class FreeSelectModal : public Modal {
public:
	FreeSelectModal();
	bool update_self(Project* p);
};

class FreeSelectFilter : public BackupFilter {
public:
	FreeSelectFilter(Project *p);
	~FreeSelectFilter();
	void run() override;
	void finalize() override;
	Shader* getShader() override;
private:
	glm::vec2 first_mousepos;
	ShaderProgram* program;
};


#endif //DEMIURGE_FREESELECTMODAL_H
