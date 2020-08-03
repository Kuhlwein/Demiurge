//
// Created by kuhlwein on 8/2/20.
//

#ifndef DEMIURGE_GRADIENTNOISE_H
#define DEMIURGE_GRADIENTNOISE_H

#include "FilterModal.h"

struct NoiseParams {
	float scale;
	float lacunarity;
	float persistence;
	int octaves;
} ;


class GradientNoiseMenu : public InstantFilterModal {
public:
	GradientNoiseMenu();
	void update_self(Project* p) override;
	std::unique_ptr<BackupFilter> makeFilter(Project* p);
private:
	NoiseParams params = {1.0,2,0.5,8};
};

class GradientNoiseFilter : public BackupFilter {
public:
	GradientNoiseFilter(Project *p, std::function<Texture *(Project *)> target, NoiseParams* params);
	~GradientNoiseFilter() override;
	void run(Project* p) override;
	bool isFinished() override;
private:
	ShaderProgram* program;
	NoiseParams* params;
};


#endif //DEMIURGE_GRADIENTNOISE_H
