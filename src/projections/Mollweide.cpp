//
// Created by kuhlwein on 7/2/20.
//

#include "Project.h"
#include "Mollweide.h"

Mollweide::Mollweide(Project *project) : AbstractCanvas(project) {

}

glm::vec2 Mollweide::inverseTransform(glm::vec2 coord) {
	float theta = asin(coord.y/sqrt(2));

	float phi = asin((2*theta+sin(2*theta))/M_PI);
	float lambda = M_PI*coord.x/(2*sqrt(2)*cos(theta));

	return glm::vec2(lambda,phi);
}

Shader* Mollweide::inverseShader() {
	return Shader::builder()
	.include(def_pi).create(R"(
vec2 inverseshader(vec2 coord) {
	float theta = asin(coord.y/sqrt(2));

    float phi = asin((2*theta+sin(2*theta))/3.14159);
    float lambda = 3.14159*coord.x/(2*sqrt(2)*cos(theta));

	if (coord.y<-sqrt(2)) discard;
    if (coord.y>sqrt(2)) discard;
    if (lambda<-3.14159) discard;
    if (lambda>3.14159) discard;

	return vec2(lambda,phi);
}
)");
}

glm::vec2 Mollweide::getScale() {
	return glm::vec2(2*sqrt(2),2*sqrt(2));
}

glm::vec2 Mollweide::getLimits() {
	return glm::vec2(1,0.5);
}
