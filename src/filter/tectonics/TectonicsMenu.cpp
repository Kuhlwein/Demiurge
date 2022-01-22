//
// Created by kuhlwein on 1/20/22.
//

#include "Project.h"
#include "TectonicsMenu.h"
#include "Tectonics.h"


void TectonicsMenu::update_self(Project *p) {

}

std::shared_ptr<BackupFilter> TectonicsMenu::makeFilter(Project *p) {
    return std::make_shared<ProgressFilter>(p, [](Project* p){return p->get_terrain();}, std::move(std::make_unique<Tectonics>(p)));
}

TectonicsMenu::TectonicsMenu() : FilterModal("Tectonics") {

}