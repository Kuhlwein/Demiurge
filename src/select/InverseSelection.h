//
// Created by kuhlwein on 7/25/20.
//

#ifndef DEMIURGE_INVERSESELECTION_H
#define DEMIURGE_INVERSESELECTION_H

#include <Menu.h>
#include <filter/Filter.h>
#include <filter/FilterModal.h>

class InverseSelection : public FilterMenu {
public:
	InverseSelection();
	void filter(Project* p) override;
	std::function<Texture *(Project *p)> targetGetter() override;
};



#endif //DEMIURGE_INVERSESELECTION_H
