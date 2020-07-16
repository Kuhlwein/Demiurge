//
// Created by kuhlwein on 7/16/20.
//

#ifndef DEMIURGE_HILLSHADE_H
#define DEMIURGE_HILLSHADE_H


#include "Appearance.h"

class Hillshade : public Appearance {
public:
	Hillshade();
	void prepare(Project* p) override;
	void unprepare(Project* p) override;
	Shader* getShader() override;
private:
	bool update_self(Project* p) override;

	Shader* shader;
};


#endif //DEMIURGE_HILLSHADE_H
