//
// Created by kuhlwein on 1/20/22.
//

#ifndef DEMIURGE_TECTONICSMENU_H
#define DEMIURGE_TECTONICSMENU_H


#include "../FilterModal.h"

class Project;

class TectonicsMenu : public FilterModal {
public:
    TectonicsMenu();
    void update_self(Project* p) override;
    std::shared_ptr<BackupFilter> makeFilter(Project* p) override;
private:

};

#endif //DEMIURGE_TECTONICSMENU_H
