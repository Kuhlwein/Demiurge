//
// Created by kuhlwein on 9/21/20.
//

#ifndef DEMIURGE_DETERRACE_H
#define DEMIURGE_DETERRACE_H

#include "Filter.h"
#include "FilterModal.h"

class Project;

class DeTerrace : public AsyncSubFilter {
public:
	DeTerrace(Project* p, Texture *target);
	//std::pair<bool,float> step(Project* p) override;
	void run() override;
private:
	Project* p;
	Texture* target;
	Texture* tex;
	int dim;
	int i = 0;

	ShaderProgram* init1;
	ShaderProgram* init2;
	ShaderProgram* program;

	std::unique_ptr<float[]> data;

	int steps = 0;
	std::vector<float> r;
	std::unique_ptr<float[]> get(glm::vec2 primary, glm::vec2 secondary);
	void updateDistance(std::unique_ptr<float[]> data);
};

class DeTerraceMenu : public FilterModal {
public:
	DeTerraceMenu();
	void update_self(Project* p) override;
	std::shared_ptr<BackupFilter> makeFilter(Project* p) override;
};


#endif //DEMIURGE_DETERRACE_H