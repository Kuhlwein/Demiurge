//
// Created by kuhlwein on 8/10/20.
//

#ifndef DEMIURGE_CPUFILTER_H
#define DEMIURGE_CPUFILTER_H


#include "FilterModal.h"

class Project;

class cpufilterMenu : public FilterModal {
public:
	cpufilterMenu();
	void update_self(Project* p) override;
	std::shared_ptr<BackupFilter> makeFilter(Project* p) override;
private:
	float exponent = 0.5;
	float factor = 1;
	float sexponent = 1;
	int dolakes;
};

class cpufilter : public AsyncSubFilter {
public:
	cpufilter(Project *p, float exponent, float slope_exponent, float factor, int dolakes);
	~cpufilter();
	void run() override;
private:
	std::unique_ptr<float[]> data;
	std::unique_ptr<float[]> selection;
	int width;
	int height;
	float level = 0.0;

	float exponent = 0.5;
	float factor = 1;
	float slope_exponent = 1;
	int dolakes;
};


#endif //DEMIURGE_CPUFILTER_H
