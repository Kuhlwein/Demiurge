//
// Created by kuhlwein on 7/5/20.
//

#include "Project.h"
#include "Equiretangular.h"

Equiretangular::Equiretangular(Project *project) : AbstractCanvas(project) {

}

glm::vec2 Equiretangular::inverseTransform(glm::vec2 coord) {
	return glm::vec2(coord);
}

Shader* Equiretangular::inverseShader() {
	return Shader::builder()
			.include(def_pi).create(R"(
vec2 inverseshader(vec2 coord) {
	return coord;
}
)");
}

glm::vec2 Equiretangular::getScale() {
	return glm::vec2(M_PI,M_PI);
}

glm::vec2 Equiretangular::getLimits() {
	return glm::vec2(1,0.5);
}