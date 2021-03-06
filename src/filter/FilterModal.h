//
// Created by kuhlwein on 7/28/20.
//

#ifndef DEMIURGE_FILTERMODAL_H
#define DEMIURGE_FILTERMODAL_H

#include <memory>
#include <Menu.h>
#include "Filter.h"

class Project;

class FilterModal : public Modal {
public:
	FilterModal(std::string title);
	bool update_FilterModal(Project* p);
	virtual void update_self(Project* p) = 0;
	virtual std::shared_ptr<BackupFilter> makeFilter(Project* p) = 0;
private:
	std::shared_ptr<BackupFilter> filter;
	bool previewing = false;
	bool isFiltering = false;
};

class FilterMenu : public Menu {
public:
	FilterMenu(std::string title);
protected:
	virtual void filter(Project* p) = 0;
	virtual std::function<Texture *(Project *p)> targetGetter() = 0;
};

class InstantFilterModal : public Modal {
public:
	InstantFilterModal(std::string title);
	bool update_InstantFilterModal(Project* p);
	virtual void update_self(Project* p) = 0;
	virtual std::unique_ptr<BackupFilter> makeFilter(Project* p) = 0;
protected:
	std::unique_ptr<BackupFilter> filter;
private:
	bool first = true;
};


#endif //DEMIURGE_FILTERMODAL_H
