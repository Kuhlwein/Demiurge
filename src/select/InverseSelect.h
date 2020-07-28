//
// Created by kuhlwein on 7/25/20.
//

#ifndef DEMIURGE_INVERSESELECT_H
#define DEMIURGE_INVERSESELECT_H

#include <Menu.h>
#include <filter/Filter.h>
#include <filter/FilterModal.h>

class InverseSelect : public FilterMenu {
public:
	InverseSelect();
	void filter(Project* p) override;
	std::function<Texture *(Project *p)> targetGetter() override;
};



#endif //DEMIURGE_INVERSESELECT_H
