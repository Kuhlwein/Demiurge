//
// Created by kuhlwein on 8/23/20.
//

#ifndef DEMIURGE_BLURSELECTION_H
#define DEMIURGE_BLURSELECTION_H


#include <filter/FilterModal.h>

class Project;

class BlurSelection : public FilterModal {
public:
	BlurSelection();
	void update_self(Project* p) override;
	std::shared_ptr<BackupFilter> makeFilter(Project* p) override;
private:
	float radius = 1.0f;
};


#endif //DEMIURGE_BLURSELECTION_H
