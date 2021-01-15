//
// Created by kuhlwein on 9/21/20.
//

#ifndef DEMIURGE_DETERRACE_H
#define DEMIURGE_DETERRACE_H

#include "Filter.h"
#include "FilterModal.h"

class Project;

class DeTerrace : public SubFilter {
public:
	DeTerrace(Project* p, Texture *target);
	std::pair<bool,float> step(Project* p) override;
private:
	Project* p;
	Texture* target;
	int i = 0;
};

class DeTerraceMenu : public FilterModal {
public:
	DeTerraceMenu();
	void update_self(Project* p) override;
	std::shared_ptr<BackupFilter> makeFilter(Project* p) override;
};


#endif //DEMIURGE_DETERRACE_H