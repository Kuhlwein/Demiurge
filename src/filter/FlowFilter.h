//
// Created by kuhlwein on 8/12/20.
//

#ifndef DEMIURGE_FLOWFILTER_H
#define DEMIURGE_FLOWFILTER_H


#include "Filter.h"
#include "FilterModal.h"

class Project;

class FlowfilterMenu : public FilterModal {
public:
	FlowfilterMenu();
	void update_self(Project* p) override;
	std::shared_ptr<BackupFilter> makeFilter(Project* p) override;
private:

};

class FlowFilter : public AsyncSubFilter {
public:
	FlowFilter();
	~FlowFilter();
	void run() override;
private:
	std::vector<int> neighbours(int pos, int dat);
	bool Nthbit(int num, int N);
	int width;
	int height;
	std::vector<float> coords;
	float level = 0.0;
};

namespace cputools2 {
	template<typename T> void threadpool(std::function<void(T a)> f,std::vector<T> arg);
}


#endif //DEMIURGE_FLOWFILTER_H
