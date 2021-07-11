//
// Created by kuhlwein on 6/29/21.
//

#ifndef DEMIURGE_TEMPERATURE_H
#define DEMIURGE_TEMPERATURE_H

#include "Filter.h"
#include "FilterModal.h"

class Project;

class TemperatureMenu : public FilterModal {
public:
	TemperatureMenu();
	void update_self(Project* p) override;
	std::shared_ptr<BackupFilter> makeFilter(Project* p) override;
private:

};

class Temperature : public AsyncSubFilter {
public:
	Temperature(Project *p);
	~Temperature();
	void run() override;
};


#endif //DEMIURGE_TEMPERATURE_H
