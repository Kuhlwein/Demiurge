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

};

class cpufilter : public AsyncSubFilter {
public:
	cpufilter(Project *p);
	void setup(Project* p) override ;
	void run() override ;
	void finalize(Project* p) override;
private:
	std::unique_ptr<float[]> data;
	std::unique_ptr<float[]> selection;
	int width;
	int height;
	float level = 0.0;
};

namespace cputools {
	template<typename T> void threadpool(std::function<void(T a)> f,std::vector<T> arg);
}


#endif //DEMIURGE_CPUFILTER_H
