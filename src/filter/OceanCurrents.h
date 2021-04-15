//
// Created by kuhlwein on 4/12/21.
//

#ifndef DEMIURGE_OCEANCURRENTS_H
#define DEMIURGE_OCEANCURRENTS_H


#include <memory>
#include "Filter.h"
#include "FilterModal.h"

class Project;

class OceanCurrentsMenu : public FilterModal {
public:
	OceanCurrentsMenu();
	void update_self(Project* p) override;
	std::shared_ptr<BackupFilter> makeFilter(Project* p) override;
private:

};

class OceanCurrents : public AsyncSubFilter {
public:
	OceanCurrents(Project *p);
	~OceanCurrents();
	void run() override;
private:
	Texture* v;
	Texture* v_scratch;
	Texture* pressure;
	Texture* scratch;

	ShaderProgram* setzero;

	void advect(Project* p);

	void divergence(Project* p);
	void jacobi(Project* p);
	void subDiv(Project* p);
};


#endif //DEMIURGE_OCEANCURRENTS_H