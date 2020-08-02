//
// Created by kuhlwein on 6/13/19.
//

#ifndef DEMIURGE_SHADER_H
#define DEMIURGE_SHADER_H

#include <string>
#include <vector>

class Shader {
public:
    class builder;
    Shader(std::string include_code, std::vector<Shader*> includes, std::string main_code);
    std::string getCode();
private:
    std::string include_code;
    std::string main_code;
    std::vector<Shader*> includes;
};

class Shader::builder {
public:
    builder include(Shader* shader);
    Shader* create(std::string include_code="",std::string main_code="");
private:
    std::vector<Shader*> includes = {};
};

static Shader* def_pi = Shader::builder()
		.create("#define M_PI 3.1415926535897932384626433832795");

static Shader* cornerCoords = Shader::builder()
		.include(def_pi)
		.create(R"(
uniform float cornerCoords[4];

vec2 to_geographic(vec2 p) {
p.x = (p.x*(cornerCoords[3]-cornerCoords[2])+cornerCoords[2]);
p.y = (p.y*(cornerCoords[1]-cornerCoords[0])+cornerCoords[0]);
return p;
}

)");

/*
 *  Vertex shaders
 */
static Shader* vertexSetup = Shader::builder()
        .create(R"(
layout (location=0) in vec3 position;
layout (location=1) in vec2 texCoord;

out vec2 st;

uniform mat4 worldMatrix;
uniform mat4 projectionMatrix;
)");

static Shader* vertex3D = Shader::builder()
        .include(vertexSetup)
        .create("",R"(
gl_Position = projectionMatrix * worldMatrix * vec4(position, 1.0);
st = texCoord;
)");

static Shader* vertex2D = Shader::builder()
        .include(vertexSetup)
        .create("",R"(
gl_Position =  vec4(position, 1.0);
st = texCoord;
)");


/*
 * Fragment shaders
 */

static Shader* fragmentBase = Shader::builder()
        .create(R"(
in vec2 st;
uniform sampler2D img;
uniform sampler2D sel;
uniform sampler2D scratch1;
uniform sampler2D scratch2;
out float fc;
)");

static Shader* fragmentColor = Shader::builder()
		.create(R"(
in vec2 st;
uniform sampler2D img;
uniform sampler2D sel;
uniform sampler2D scratch1;
uniform sampler2D scratch2;
out vec4 fc;
)");

static Shader* mouseLocation = Shader::builder()
        .create(R"(
uniform vec2 mouse;
uniform vec2 mousePrev;
uniform float brush_size;
)");

static Shader* copy_img = Shader::builder()
        .include(fragmentBase)
        .create("uniform sampler2D to_be_copied;",R"(
fc = texture(to_be_copied, st).r;
)");

static Shader* fix_0 = Shader::builder()
		.include(fragmentBase)
		.create("",R"(
fc = texture(img, st).r;
if (fc>-0 && fc<1e-9) fc=0.0;
)");

static Shader* fragment_set = Shader::builder()
        .include(fragmentBase)
        .create(R"(
uniform float value=0;
)",R"(
fc = value;
)");

/*
 * Drawing map
 */

static Shader* draw_grayscale = Shader::builder()
		.include(fragmentColor)
		.create("",R"(
fc = (texture(img, st_p).rrrr+1)*0.5;
)");

//static Shader* draw_gradient = Shader::builder()
//		.include(fragmentColor)
//        .create(R"(
//uniform sampler2D gradient_land;
//uniform sampler2D gradient_ocean;
//)",R"(
//float h = texture(img, st_p).r;
//if (h>0) {
//    fc = texture(gradient_land,vec2(h,0));
//} else {
//    fc = texture(gradient_ocean,vec2(1+h,0));
//}
//)");

static Shader* deltaxy = Shader::builder()
		.create(R"(
float dx = dFdx(st.x);
float dy = dFdy(st.y);
)");


static Shader* draw_normal = Shader::builder()
		.include(fragmentColor)
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

fc = vec4(normalv,0);
)");

static Shader* brush_outline = Shader::builder()
		.include(mouseLocation)
        .create(R"(
void draw_brush_outline(inout vec4 fc, in vec2 st) {
float r = geodistance(mouse,st,textureSize(img,0));
float delta = 2*length(vec2(dFdx(r),dFdy(r)));
if (r<brush_size && r>brush_size-delta) {
  //fc = vec4(1,1,1,0);
float w = abs(r-(brush_size-0.5*delta))/(0.5*delta);
    fc = fc*(w) + vec4(1,1,1,0)*(1-w);
}
}
)","draw_brush_outline(fc,st_p);");


static Shader* graticules = Shader::builder()
		.include(def_pi)
		.include(cornerCoords)
		.create(R"(
void draw_graticules(inout vec4 fc, in vec2 st, in float grat, in vec4 grat_color) {

st = to_geographic(st)/M_PI*180;

vec2 dx = dFdx(st);
vec2 dy = dFdy(st);
dx.x = min(abs(dx.x),360-abs(dx.x));
dy.x = min(abs(dy.x),360-abs(dy.x));
float xdiff = 1.2*length(vec2(dx.x,dy.x));
float ydiff = 1.2*length(vec2(dx.y,dy.y));

float absdiff = mod(abs(st.x),grat);
float r = min(absdiff,grat-absdiff);
float w = 1-r/(xdiff);
if (r<xdiff) fc = fc*(1-w*grat_color.w) + grat_color*(w*grat_color.w);

absdiff = mod(abs(st.y),grat);
r = min(absdiff,grat-absdiff);
w = 1-r/(ydiff);
if (r<ydiff) fc = fc*(1-w*grat_color.w) + grat_color*(w*grat_color.w);


//if (abs(dx.x)>360) fc = vec4(1,0,0,0);

}
)","");

static Shader* selection_outline = Shader::builder()
        .create(R"(
uniform float u_time;
void draw_selection_outline(inout vec4 fc, in vec2 st) {
	float dx2 = dFdx(st.x);
    float dy2 = dFdy(st.y);
    float x1 = texture(sel, st-vec2(dx2,0)).r;
    float x2 = texture(sel, st+vec2(dx2,0)).r;
    float y1 = texture(sel, st-vec2(0,dy2)).r;
    float y2 = texture(sel, st+vec2(0,dy2)).r;


    float test = round(mod(gl_FragCoord.x/8-gl_FragCoord.y/8+u_time,1));

    if (bool(x1) != bool(x2)) fc = vec4(test,test,test,0);
    if (bool(y1) != bool(y2)) fc = vec4(test,test,test,0);
}
)","draw_selection_outline(fc,st_p);");


static Shader* texturespace_gradient = Shader::builder()
		.create(R"(
void get_texture_gradient(inout float delta_x, inout float delta_y) {
vec2 texture_stepsize_x = vec2(1,0)/textureSize(img,0);
vec2 texture_stepsize_y = vec2(0,1)/textureSize(img,0);

float x1 = texture(img, projection(st)-texture_stepsize_x).r;
float x2 = texture(img, projection(st)+texture_stepsize_x).r;
float y1 = texture(img, projection(st)-texture_stepsize_y).r;
float y2 = texture(img, projection(st)+texture_stepsize_y).r;

delta_x = x1-x2;
delta_y = y2-y1;
}
float delta_x;
float delta_y;
)","get_texture_gradient(delta_x,delta_y);");

static Shader* get_aspect = Shader::builder()
		.include(def_pi)
		.include(texturespace_gradient)
		.create(R"(
float get_aspect() {
return M_PI-atan(delta_y, -(delta_x));
}
)","");


static Shader* get_slope = Shader::builder()
		.include(def_pi)
		.include(texturespace_gradient)
		.create(R"(
float get_slope(float z_factor) {
return atan(z_factor * sqrt(pow(delta_x,2) + pow(delta_y,2)));
}
)","");


#endif //DEMIURGE_SHADER_H
