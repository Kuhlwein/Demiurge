//
// Created by kuhlwein on 8/2/20.
//

#ifndef DEMIURGE_GRADIENTNOISE_H
#define DEMIURGE_GRADIENTNOISE_H

#include "FilterModal.h"

enum noise_mode {DEFAULT, RIDGED, BILLOWY, IQNoise, SWISS, Jordan};
//IQ-noise : supress details on slopes
//Swiss : Ridged with smoothed valleys, damp on height
//Jordan : billowy-ish, damp on gradient?

struct NoiseParams {
	float scale;
	float lacunarity;
	float persistence;
	float min;
	float max;
	int octaves;
	int seed;
	float warp;
	noise_mode mode;
} ;


class GradientNoiseMenu : public FilterModal {
public:
	GradientNoiseMenu();
	void update_self(Project* p) override;
	std::shared_ptr<BackupFilter> makeFilter(Project* p);
private:
	NoiseParams params = {1.0,2,0.5,-1,1,8,0,0.0f,DEFAULT};
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
