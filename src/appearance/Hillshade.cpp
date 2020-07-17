//
// Created by kuhlwein on 7/16/20.
//

#include <Shader.h>
#include "Hillshade.h"

Hillshade::Hillshade() : Appearance("Hillshade") {
	shader = Shader::builder()
			.include(fragmentColor)
			.include(deltaxy)
			.include(def_pi)
			.create(R"(
vec2 deltax = vec2(1,0)/textureSize(img,0);
vec2 deltay = vec2(0,1)/textureSize(img,0);

float x1 = texture(img, projection(st)-deltax).r;
float x2 = texture(img, projection(st)+deltax).r;
float y1 = texture(img, projection(st)-deltay).r;
float y2 = texture(img, projection(st)+deltay).r;


)",R"(
vec3 normalv = vec3(x1-x2,y2-y1,0.1);
normalv = (normalv/length(normalv)+1)/2;

float z_factor = 5.0f;
float slope =  atan(z_factor * sqrt(pow(x1-x2,2) + pow(y2-y1,2)));

float aspect = atan(y2-y1, -(x1-x2));
aspect=aspect+M_PI;
//aspect = aspect/2/M_PI;

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
