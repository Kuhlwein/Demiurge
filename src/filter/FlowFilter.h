//
// Created by kuhlwein on 8/12/20.
//

#ifndef DEMIURGE_FLOWFILTER_H
#define DEMIURGE_FLOWFILTER_H


#include "Filter.h"
#include "FilterModal.h"
#include <unordered_map>

class Project;

class FlowfilterMenu : public FilterModal {
public:
	FlowfilterMenu();
	void update_self(Project* p) override;
	std::shared_ptr<BackupFilter> makeFilter(Project* p) override;
private:
	float exponent=1;
	float preblur=0.5;
};

class FlowFilter : public AsyncSubFilter {
public:
	FlowFilter(float preblur,float exponent, bool lakeflag=false);
	~FlowFilter();
	void run() override;
private:
	struct pass {
		float h;
		int from; //Which lake is the flow from
		int tolocation; //In self
	};
	float preblur;
	float exponent;
	bool lakeflag;

	std::vector<int> neighbours(int pos, int dat);
	bool Nthbit(int num, int N);
	int width;
	int height;
	std::vector<float> coords;
	float circumference;
	float level = 0.0;
	std::unique_ptr<float[]> data;
	std::unique_ptr<float[]> lakeID;

	std::mutex mtx;
	std::vector<std::pair<int,int>> ofInterest;

	void findMagicNumbers();
	void findPointsOfInterest();
	void indexLakes(std::vector<std::vector<int>> *lakes);
	void assignLakeIds(std::vector<std::vector<int>> *lakes);
	void findAllConnections(std::vector<std::vector<int>> *lakes);
	void solvingConnections(std::vector<std::vector<int>> *lakes, std::unordered_map<int,pass> &connections);
	void calculateflow(std::vector<std::vector<int>> *lakes, std::unordered_map<int,pass> &connections);

	std::function<bool(const pass&, const pass&)> comp = [](const pass& c1, const pass& c2){return c1.h<c2.h;};
	std::unordered_map<int,std::set<pass,decltype(comp)>*> passes; //map lake to set of passes, sorted by height. Passes are from some other lake to key of map //TODO MUST BE DELETED MANUALLY!
};


#endif //DEMIURGE_FLOWFILTER_H
