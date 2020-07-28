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
	virtual std::shared_ptr<Filter> makeFilter(Project* p) = 0;
private:
	std::shared_ptr<Filter> filter;
	bool previewing = false;
};

class FilterMenu : public Menu {
public:
	FilterMenu(std::string title);
protected:
	virtual void filter(Project* p) = 0;
	virtual std::function<Texture *(Project *p)> targetGetter() = 0;
};


#endif //DEMIURGE_FILTERMODAL_H
