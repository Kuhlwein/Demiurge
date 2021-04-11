//
// Created by kuhlwein on 4/11/21.
//

#ifndef DEMIURGE_VECTORFIELD_H
#define DEMIURGE_VECTORFIELD_H


#include "Appearance.h"

class VectorField : public Appearance {
public:
	VectorField();
	void prepare(Project* p) override;
	void unprepare(Project* p) override;
	Shader* getShader() override;
private:
	bool update_self(Project* p) override;
	Shader* shader;
};


#endif //DEMIURGE_VECTORFIELD_H
