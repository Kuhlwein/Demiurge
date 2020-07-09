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

static Shader* cornerCoords = Shader::builder()
		.create("uniform float cornerCoords[4];");

static Shader* def_pi = Shader::builder()
		.create("#define M_PI 3.1415926535897932384626433832795");

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

static Shader* draw_gradient = Shader::builder()
		.include(fragmentColor)
        .create(R"(
uniform sampler2D gradient_land;
uniform sampler2D gradient_ocean;
)",R"(
float h = texture(img, st_p).r;
if (h>0) {
    fc = texture(gradient_land,vec2(h,0));
} else {
    fc = texture(gradient_ocean,vec2(1+h,0));
}
)");

static Shader* deltaxy = Shader::builder()
		.create(R"(
float dx = dFdx(st.x);
float dy = dFdy(st.y);
)");


static Shader* draw_normal = Shader::builder()
		.include(fragmentColor)
		.include(deltaxy)
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

static Shader* blur5 = Shader::builder()
		.create(R"(
float blur5(sampler2D image, vec2 uv, vec2 direction) {
  float color = 0.0f;
  vec2 resolution = textureSize(image,0);
  vec2 off1 = vec2(1.3333333333333333) * direction;
  color += texture2D(image, uv).r * 0.29411764705882354;
  color += texture2D(image, uv + (off1 / resolution)).r * 0.35294117647058826;
  color += texture2D(image, uv - (off1 / resolution)).r * 0.35294117647058826;
  return color;
}
)");

static Shader* blur13 = Shader::builder()
		.create(R"(
float blur13(sampler2D image, vec2 uv, vec2 direction) {
  float color = 0.0f;
  vec2 resolution = textureSize(image,0);
  vec2 off1 = vec2(1.411764705882353) * direction;
float phi = (uv.y-0.5)*3.14159;
off1.x = off1.x/cos(abs(phi)+0.01);
  vec2 off2 = vec2(3.2941176470588234) * direction;
off2.x = off2.x/cos(abs(phi)+0.01);
  vec2 off3 = vec2(5.176470588235294) * direction;
off3.x = off3.x/cos(abs(phi)+0.01);
  color += texture2D(image, uv).r * 0.1964825501511404;
  color += texture2D(image, offset(uv, off1,resolution)).r * 0.2969069646728344;
  color += texture2D(image, offset(uv,-off1,resolution)).r * 0.2969069646728344;
  color += texture2D(image, offset(uv, off2,resolution)).r * 0.09447039785044732;
  color += texture2D(image, offset(uv,-off2,resolution)).r * 0.09447039785044732;
  color += texture2D(image, offset(uv, off3,resolution)).r * 0.010381362401148057;
  color += texture2D(image, offset(uv,-off3,resolution)).r * 0.010381362401148057;
  return color;
}
)");



static Shader* brush_outline = Shader::builder()
        .include(deltaxy)
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
void draw_graticules(inout vec4 fc, in vec2 st) {

st.x = (st.x*(cornerCoords[3]-cornerCoords[2])+cornerCoords[2])/M_PI/2*360;
st.y = (st.y*(cornerCoords[1]-cornerCoords[0])+cornerCoords[0])/M_PI*180;

float grat = 15;

vec2 dx = dFdx(st);
vec2 dy = dFdy(st);
dx.x = min(abs(dx.x),360-abs(dx.x));
dy.x = min(abs(dy.x),360-abs(dy.x));
float xdiff = 1.2*length(vec2(dx.x,dy.x));
float ydiff = 1.2*length(vec2(dx.y,dy.y));

float absdiff = mod(abs(st.x),grat);
float r = min(absdiff,grat-absdiff);
float w = r/(xdiff);
if (r<xdiff) fc = fc*(w) + vec4(1,1,1,0)*(1-w);

absdiff = mod(abs(st.y),grat);
r = min(absdiff,grat-absdiff);
w = r/(ydiff);
if (r<ydiff) fc = fc*(w) + vec4(1,1,1,0)*(1-w);


if (abs(dx.x)>360) fc = vec4(1,0,0,0);

}
)","draw_graticules(fc,st_p);");

static Shader* selection_outline = Shader::builder()
        .include(deltaxy)
        .create(R"(
uniform float u_time;
void draw_selection_outline(inout vec4 fc, in vec2 st) {
	float dx2 = dFdx(st.x);
    float dy2 = dFdy(st.y);
    float x1 = texture(sel, st-vec2(dx2,0)).r;
    float x2 = texture(sel, st+vec2(dx2,0)).r;
    float y1 = texture(sel, st-vec2(0,dy2)).r;
    float y2 = texture(sel, st+vec2(0,dy2)).r;

    float k = round(dx*20000);
    float test = round(mod(gl_FragCoord.x/8-gl_FragCoord.y/8+u_time,1));

    if (abs(x1-mod(x1,0.2)-(x2-mod(x2,0.2)))>0) fc = vec4(test,test,test,0);
    if (abs(y1-mod(y1,0.2)-(y2-mod(y2,0.2)))>0) fc = vec4(test,test,test,0);
}
)","draw_selection_outline(fc,st_p);");








#endif //DEMIURGE_SHADER_H
