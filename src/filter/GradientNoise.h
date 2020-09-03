//
// Created by kuhlwein on 8/2/20.
//

#ifndef DEMIURGE_GRADIENTNOISE_H
#define DEMIURGE_GRADIENTNOISE_H

#include "FilterModal.h"

enum noise_mode {DEFAULT, RIDGED, BILLOWY, IQNoise, SWISS, Jordan,Plateaus};
//IQ-noise : supress details on slopes //Gradient supressed
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
	NoiseParams params = {1.0,2,0.5,-10,10,8,0,0.0f,DEFAULT};
	Shader* blendmode;
};

class GradientNoiseFilter : public SubFilter {
public:
	GradientNoiseFilter(NoiseParams params, Shader* blendmode);
	~GradientNoiseFilter();
	std::pair<bool,float> step(Project* p) override;
private:
	ShaderProgram* program;
	NoiseParams params;
	Shader* blendmode;
};


#endif //DEMIURGE_GRADIENTNOISE_H
