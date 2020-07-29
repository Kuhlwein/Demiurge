//
// Created by kuhlwein on 7/25/20.
//

#ifndef DEMIURGE_ALLSELECT_H
#define DEMIURGE_ALLSELECT_H

#include <Menu.h>
#include <filter/Filter.h>
#include <filter/FilterModal.h>

class AllSelect : public FilterMenu {
public:
	void filter(Project* p) override;
	std::function<Texture *(Project *p)> targetGetter() override;
	AllSelect();
};


#endif //DEMIURGE_ALLSELECT_H
