//
// Created by kuhlwein on 7/30/20.
//

#ifndef DEMIURGE_OFFSETMENU_H
#define DEMIURGE_OFFSETMENU_H


#include "Filter.h"
#include "FilterModal.h"

class OffsetMenu : public InstantFilterModal {
public:
	OffsetMenu();
	void update_self(Project* p) override;
	std::unique_ptr<BackupFilter> makeFilter(Project* p);
private:
	float offset;
};

class OffsetFilter : public BackupFilter {
public:
	OffsetFilter(Project* p, float* offset);
	~OffsetFilter();
	void run(Project* p) override;
	bool isFinished() override;
private:
	ShaderProgram* program;
	float* offset;
};

#endif //DEMIURGE_OFFSETMENU_H
