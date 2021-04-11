//
// Created by kuhlwein on 7/10/20.
//

#include "Project.h"
#include "Sinusoidal.h"

Sinusoidal::Sinusoidal(Project *project) : AbstractCanvas(project) {

}

glm::vec2 Sinusoidal::inverseTransform(glm::vec2 coord) {

	float phi = coord.y;
	float lambda = coord.x/cos(phi);

	return glm::vec2(lambda,phi);
}

Shader* Sinusoidal::inverseShader() {
	return Shader::builder()
			.include(def_pi).create(R"(
vec2 inverseshader(vec2 coord, inout bool outOfBounds) {

    float phi = coord.y;
    float lambda = coord.x/cos(phi);

    if (lambda<-3.14159) outOfBounds=true;
    if (lambda>3.14159) outOfBounds=true;


	return vec2(lambda,phi);
}
)");
}

glm::vec2 Sinusoidal::getScale() {
	return glm::vec2(M_PI,M_PI);
}

glm::vec2 Sinusoidal::getLimits() {
	return glm::vec2(1,0.5);
}

bool Sinusoidal::isInterruptible() {
	return true;
}
