//
// Created by kuhlwein on 8/23/20.
//

#ifndef DEMIURGE_BORDERSELECTION_H
#define DEMIURGE_BORDERSELECTION_H


#include <filter/FilterModal.h>
class Project;

class BorderMenu : public FilterModal {
public:
	BorderMenu();
	void update_self(Project* p) override;
	std::shared_ptr<BackupFilter> makeFilter(Project* p) override;
private:
	float radius = 1.0f;
};


#endif //DEMIURGE_BORDERSELECTION_H
