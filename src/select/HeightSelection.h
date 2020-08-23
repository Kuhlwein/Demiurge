//
// Created by kuhlwein on 8/23/20.
//

#ifndef DEMIURGE_HEIGHTSELECTION_H
#define DEMIURGE_HEIGHTSELECTION_H


#include <filter/FilterModal.h>

class HeightSelection : public FilterModal {
public:
	HeightSelection();
	void update_self(Project* p) override;
	std::shared_ptr<BackupFilter> makeFilter(Project* p) override;
private:
	Shader* mode;
};

class HeightSelectFilter : public BackupFilter {
public:
	HeightSelectFilter(Project *p, Shader *mode);
	~HeightSelectFilter() override;
	void run(Project* p) override;
	Shader* getShader() override;
	bool isFinished() override;
private:
	ShaderProgram* program;
	Shader* mode;
	bool finished = false;
};


#endif //DEMIURGE_HEIGHTSELECTION_H
