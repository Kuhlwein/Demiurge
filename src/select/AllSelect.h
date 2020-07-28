//
// Created by kuhlwein on 7/25/20.
//

#ifndef DEMIURGE_ALLSELECT_H
#define DEMIURGE_ALLSELECT_H

#include <Menu.h>
#include <filter/Filter.h>

class AllSelect : public Modal {
public:
	AllSelect();
	bool update_self(Project* p);
};

class SelectAllFilter : public BackupFilter {
public:
	SelectAllFilter(Project *p);
	~SelectAllFilter();
	void run() override;
	//void finalize() override;
	bool isFinished() override;
private:
	ShaderProgram* program;
};


#endif //DEMIURGE_ALLSELECT_H
