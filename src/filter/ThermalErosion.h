//
// Created by kuhlwein on 8/16/20.
//

#ifndef DEMIURGE_THERMALEROSION_H
#define DEMIURGE_THERMALEROSION_H

#include "FilterModal.h"

class Project;

class ThermalErosion : public SubFilter {
public:
	ThermalErosion();
	std::pair<bool,float> step(Project* p) override;
};

class ThermalErosionMenu : public FilterModal {
public:
	ThermalErosionMenu();
	void update_self(Project* p) override;
	std::shared_ptr<BackupFilter> makeFilter(Project* p) override;
};


#endif //DEMIURGE_THERMALEROSION_H
