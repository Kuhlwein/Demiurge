//
// Created by kuhlwein on 7/31/20.
//

#ifndef DEMIURGE_MORPHOLOGICAL_H
#define DEMIURGE_MORPHOLOGICAL_H

#include <memory>
#include "Filter.h"
#include "FilterModal.h"

class Project;

class ErodeMenu : public FilterModal {
public:
	ErodeMenu();
	void update_self(Project* p) override;
	std::shared_ptr<Filter> makeFilter(Project* p) override;
private:
	float radius = 1.0f;
};

class Erode : public SubFilter {
public:
	Erode(Project *p, float radius, Texture *target);
	std::pair<bool,float> step(Project* p) override;
private:
	ShaderProgram *erodeProgram;
	Texture* target;
	int steps = 0;
	std::vector<float> r;
};


#endif //DEMIURGE_MORPHOLOGICAL_H
