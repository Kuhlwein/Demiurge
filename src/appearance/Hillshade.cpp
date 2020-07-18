//
// Created by kuhlwein on 7/16/20.
//

#include <Shader.h>
#include "Hillshade.h"

Hillshade::Hillshade() : Appearance("Hillshade") {
	shader = Shader::builder()
			.include(fragmentColor)
			.include(def_pi)
			.include(texturespace_gradient)
			.create(R"(
)",R"(

vec3 normalv = vec3(delta_x,delta_y,0.1);
normalv = (normalv/length(normalv)+1)/2;

float z_factor = 5.0f;
float slope =  atan(z_factor * sqrt(pow(delta_x,2) + pow(delta_y,2)));

float aspect = atan(delta_y, -(delta_x));
aspect=aspect+M_PI;

float altitude = M_PI/4;
float zenith = M_PI/2-altitude;

float azimuth = 315;
float azimuth_math = (azimuth - 90)/180*M_PI;

float hillshade = ((cos(zenith) * cos(slope)) +
             (sin(zenith) * sin(slope) * cos(azimuth_math - aspect)));


//fc = vec4(normalv,0);
//fc = vec4(hillshade,hillshade,hillshade,0);
//fc = vec4(slope,slope,slope,0);
//fc = vec4(aspect,aspect,aspect,0);
vec4 k = texture(gradient_ocean_0,vec2(hillshade,0));
fc = fc*(1-k.a) + k*(k.a);
)");
}

bool Hillshade::update_self(Project *p) {
	return true;
}

Shader *Hillshade::getShader() {
	return shader;
}

void Hillshade::unprepare(Project *p) {

}

void Hillshade::prepare(Project *p) {

}
