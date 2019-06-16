//
// Created by kuhlwein on 6/13/19.
//

#include <iostream>
#include "Shader.h"
#include <algorithm>

Shader::Shader(std::string code, std::vector<Shader *> includes) {
    this->code = code;
    this->includes = includes;
}

std::string Shader::getCode(){
    std::string finalCode = "#version 430";
    for (Shader* s : includes) {
        finalCode += "\n\n" + s->code;
    }
    return finalCode + "\n\nvoid main () {\n" + code + "\n}";
}

Shader::builder Shader::builder::include(Shader *shader) {
    std::cout << "a\n";
    for (Shader* s : shader->includes) {
        if(std::find(includes.begin(), includes.end(), s) == includes.end()) includes.push_back(s);
    }
    if(std::find(includes.begin(), includes.end(), shader) == includes.end()) includes.push_back(shader);
    return *this;
}

Shader *Shader::builder::create(std::string code) {
    return new Shader(code,includes);
}
