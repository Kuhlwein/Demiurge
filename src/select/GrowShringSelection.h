//
// Created by kuhlwein on 8/23/20.
//

#ifndef DEMIURGE_GROWSHRINGSELECTION_H
#define DEMIURGE_GROWSHRINGSELECTION_H

#include <filter/FilterModal.h>

class Project;

class GrowShrinkMenu : public FilterModal {
public:
	GrowShrinkMenu();
	void update_self(Project* p) override;
	std::shared_ptr<BackupFilter> makeFilter(Project* p) override;
private:
	float radius = 1.0f;
	int current = 1;
};


#endif //DEMIURGE_GROWSHRINGSELECTION_H
