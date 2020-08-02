//
// Created by kuhlwein on 8/2/20.
//

#ifndef DEMIURGE_GRADIENTNOISE_H
#define DEMIURGE_GRADIENTNOISE_H

#include "FilterModal.h"

class GradientNoiseMenu : public InstantFilterModal {
public:
	GradientNoiseMenu();
	void update_self(Project* p) override;
	std::unique_ptr<BackupFilter> makeFilter(Project* p);
private:
	float scale=1;
	int octaves;
};

class GradientNoiseFilter : public BackupFilter {
public:
	GradientNoiseFilter(Project *p, std::function<Texture *(Project *)> target, float* scale);
	~GradientNoiseFilter() override;
	void run(Project* p) override;
	bool isFinished() override;
private:
	ShaderProgram* program;
	float* scale;
};


#endif //DEMIURGE_GRADIENTNOISE_H
