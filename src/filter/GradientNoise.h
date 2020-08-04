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
	int seed;
} ;


class GradientNoiseMenu : public FilterModal {
public:
	GradientNoiseMenu();
	void update_self(Project* p) override;
	std::shared_ptr<BackupFilter> makeFilter(Project* p);
private:
	NoiseParams params = {1.0,2,0.5,8};
};

class GradientNoiseFilter : public SubFilter {
public:
	GradientNoiseFilter(Project *p, Texture* target, NoiseParams params);
	~GradientNoiseFilter();
	std::pair<bool,float> step(Project* p) override;
private:
	ShaderProgram* program;
	NoiseParams params;
};


#endif //DEMIURGE_GRADIENTNOISE_H
