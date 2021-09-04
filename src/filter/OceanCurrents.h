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
	float pressure = 100.0f;
};

class OceanCurrents : public AsyncSubFilter {
public:
	OceanCurrents(Project *p, float pressure);
	~OceanCurrents();
	void run() override;
private:
	Texture* v;
	Texture* v_scratch;
	Texture* pressure;
	Texture* scratch;
	Texture* divw;
	Texture* divw_showcase;

	ShaderProgram* setzero;
	ShaderProgram *jacobiProgram;
	Shader* vectorShader;

	int jacobi_iterations;

	void advect(Project* p);

	void divergence(Project* p, Texture* divw);
	void jacobi();
	void subDiv(Project* p);
	void diffusion(Project* p);

	void resize (int width, int height, Project* p);

	float pressurefactor = 100.0f;
};


#endif //DEMIURGE_OCEANCURRENTS_H
