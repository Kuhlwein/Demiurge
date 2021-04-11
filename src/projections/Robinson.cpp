//
// Created by kuhlwein on 8/9/20.
//

#include <iostream>
#include "Robinson.h"

Robinson::Robinson(Project *project) : AbstractCanvas(project) {

}

glm::vec2 Robinson::inverseTransform(glm::vec2 coord) {
	float phiofy[] = {-0.010229047032682126, 80.29654191024038, 4.4182059926979615, -9.482454267304215, -2.273688885131101, 5.7531702276094645, 9.123630935057466, 8.03779851994844, 4.225229524360806, -0.5536195511397848, -4.935999809442544, -8.000253639940851, -9.191625360964318, -8.228077452618464, -5.017647716143937, 0.4056148595412977, 7.928403995625608, 17.39105788291159};

	float y = 1;
	float phi = phiofy[0];
	for (int i=1; i<IM_ARRAYSIZE(phiofy); i++) {
		y=y*glm::abs(coord.y)/1.3523;
		phi += phiofy[i]*y;
	}

	float xofphi[] = {1.0000121679737832, -0.00019002309314508636, -2.49324010104246e-06, -4.555004740308677e-06, 2.8379397871980405e-07, -9.488976528680172e-09, 1.6197731015047832e-10, -1.357953005850529e-12, 4.453521631460094e-15};
	float x = 1;
	float lambda = xofphi[0];
	for (int i=1; i<IM_ARRAYSIZE(xofphi); i++) {
		x=x*glm::abs(phi);
		lambda += xofphi[i]*x;
	}
	lambda = coord.x/0.8487/lambda;
	phi = glm::sign(coord.y)*phi/180*M_PI;

	return glm::vec2(lambda,phi);
}

Shader* Robinson::inverseShader() {
	return Shader::builder()
			.include(def_pi).create(R"(
vec2 inverseshader(vec2 coord, inout bool outOfBounds) {
	float phiofy[] = float[](0, 80.29654191024038, 4.4182059926979615, -9.482454267304215, -2.273688885131101, 5.7531702276094645, 9.123630935057466, 8.03779851994844, 4.225229524360806, -0.5536195511397848, -4.935999809442544, -8.000253639940851, -9.191625360964318, -8.228077452618464, -5.017647716143937, 0.4056148595412977, 7.928403995625608, 17.39105788291159);

	float y = 1;
	float phi = phiofy[0];
	for (int i=1; i<phiofy.length; i++) {
		y=y*abs(coord.y)/1.3523;
		phi += phiofy[i]*y;
	}

	float xofphi[] = float[](1.0000121679737832, -0.00019002309314508636, -2.49324010104246e-06, -4.555004740308677e-06, 2.8379397871980405e-07, -9.488976528680172e-09, 1.6197731015047832e-10, -1.357953005850529e-12, 4.453521631460094e-15);
	float x = 1;
	float lambda = xofphi[0];
	for (int i=1; i<xofphi.length; i++) {
		x=x*abs(phi);
		lambda += xofphi[i]*x;
	}
    lambda = coord.x/0.8487/lambda;
	phi = sign(coord.y)*phi/180*M_PI;

	return vec2(lambda,phi);
}
)");
}

glm::vec2 Robinson::getScale() {
	return glm::vec2(1,1);
}

glm::vec2 Robinson::getLimits() {
	return glm::vec2(M_PI*0.8487,1.3523);
}

bool Robinson::isInterruptible() {
	return false;
}

