//
// Created by kuhlwein on 4/11/20.
//

#ifndef DEMIURGE_SELECTION_H
#define DEMIURGE_SELECTION_H

#include <filter/FilterModal.h>

class Project;
class Shader;

namespace selection {
	Shader* selection_mode();
	std::vector<Menu*> get_selection_menu();
	bool by_height(Project* p);
}


class GrowShrinkMenu : public FilterModal {
public:
	GrowShrinkMenu();
	void update_self(Project* p) override;
	std::shared_ptr<Filter> makeFilter(Project* p) override;
private:
	float radius = 1.0f;
	int current = 1;
};

class BorderMenu : public FilterModal {
public:
	BorderMenu();
	void update_self(Project* p) override;
	std::shared_ptr<Filter> makeFilter(Project* p) override;
private:
	float radius = 1.0f;
};

class BlurSelection : public FilterModal {
public:
	BlurSelection();
	void update_self(Project* p) override;
	std::shared_ptr<Filter> makeFilter(Project* p) override;
private:
	float radius = 1.0f;
};

#endif //DEMIURGE_SELECTION_H
