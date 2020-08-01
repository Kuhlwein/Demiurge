//
// Created by kuhlwein on 7/31/20.
//

#ifndef DEMIURGE_SCALEMENU_H
#define DEMIURGE_SCALEMENU_H


#include "Filter.h"
#include "FilterModal.h"

class ScaleMenu : public InstantFilterModal {
public:
	ScaleMenu();
	void update_self(Project* p) override;
	std::unique_ptr<BackupFilter> makeFilter(Project* p);
private:
	float scale=1;
};

class ScaleFilter : public BackupFilter {
public:
	ScaleFilter(Project* p, float* scale);
	~ScaleFilter();
	void run(Project* p) override;
	bool isFinished() override;
private:
	ShaderProgram* program;
	float* scale;
};


#endif //DEMIURGE_SCALEMENU_H
