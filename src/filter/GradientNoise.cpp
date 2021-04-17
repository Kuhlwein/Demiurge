//
// Created by kuhlwein on 8/2/20.
//

#include "Project.h"
#include <imgui/imgui.h>
#include <iostream>
#include <random>
#include <glm/glm/ext.hpp>
#include "GradientNoise.h"


GradientNoiseMenu::GradientNoiseMenu() : FilterModal("Gradient noise") {

}

void GradientNoiseMenu::update_self(Project *p) {
	//operation
	blendmode = filter::blendMode();

	ImGui::DragFloatRange2("Limits",&params.min,&params.max,0.01f,0.0f,0.0f,"%.3f km");
	static int current;
	const char* items[] = {"Default","Ridged","Billowy","Gradient supressed","Mountains","Hills","Plateaus"};
	if(ImGui::Combo("Mode",&current,items,IM_ARRAYSIZE(items))) {
		switch (current) {
			case 0: params.mode = DEFAULT; break;
			case 1: params.mode = RIDGED; break;
			case 2: params.mode = BILLOWY; break;
			case 3: params.mode = IQNoise; break;
			case 4: params.mode = SWISS; break;
			case 5: params.mode = Jordan; break;
			case 6: params.mode = Plateaus; break;
		}
	};
	//Mode

	ImGui::Separator();

	ImGui::DragInt("Seed", &params.seed,0.01f,0,0);
	ImGui::DragFloat("Scale", &params.scale, 0.001f, 0, 0, "%.4f", 1.0f);
	ImGui::DragInt("Octaves", &params.octaves,0.05f,1,64);
	ImGui::DragFloat("Lacunarity", &params.lacunarity, 0.01f/10, 0, 0, "%.4f", 1.0f);
	ImGui::DragFloat("Persistence", &params.persistence, 0.01f/10, 0, 0, "%.4f", 1.0f);
	ImGui::DragFloat("Domain warp", &params.warp, 0.01f/10, 0, 0, "%.4f", 1.0f);
}

std::shared_ptr<BackupFilter> GradientNoiseMenu::makeFilter(Project *p) {
	return std::make_shared<ProgressFilter>(p, [](Project* p){return p->get_terrain();},std::move(std::make_unique<GradientNoiseFilter>(params,blendmode)));
}

GradientNoiseFilter::GradientNoiseFilter(NoiseParams params, Shader* blendmode) : SubFilter() {
	this->params = params;
	this->blendmode = blendmode;
}



GradientNoiseFilter::~GradientNoiseFilter() {

}

std::pair<bool, float> GradientNoiseFilter::step(Project *p) {
	Shader* noise = Shader::builder()
			.include(fragmentBase)
			.include(cornerCoords)
			.create(R"(
//
// Description : Array and textureless GLSL 2D/3D/4D simplex
//               noise functions.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : stegu
//     Lastmod : 20150104 (JcBernack)
//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//               Distributed under the MIT License. See LICENSE file.
//               https://github.com/ashima/webgl-noise
//               https://github.com/stegu/webgl-noise
//

vec3 mod289(vec3 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x) {
     return mod289(((x*34.0)+1.0)*x);
}

vec4 taylorInvSqrt(vec4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}

float snoise(vec3 v, out vec3 gradient)
{
  const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
  const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

// First corner
  vec3 i  = floor(v + dot(v, C.yyy) );
  vec3 x0 =   v - i + dot(i, C.xxx) ;

// Other corners
  vec3 g = step(x0.yzx, x0.xyz);
  vec3 l = 1.0 - g;
  vec3 i1 = min( g.xyz, l.zxy );
  vec3 i2 = max( g.xyz, l.zxy );

  //   x0 = x0 - 0.0 + 0.0 * C.xxx;
  //   x1 = x0 - i1  + 1.0 * C.xxx;
  //   x2 = x0 - i2  + 2.0 * C.xxx;
  //   x3 = x0 - 1.0 + 3.0 * C.xxx;
  vec3 x1 = x0 - i1 + C.xxx;
  vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
  vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

// Permutations
  i = mod289(i);
  vec4 p = permute( permute( permute(
             i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0 ))
           + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

// Gradients: 7x7 points over a square, mapped onto an octahedron.
// The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
  float n_ = 0.142857142857; // 1.0/7.0
  vec3  ns = n_ * D.wyz - D.xzx;

  vec4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)

  vec4 x_ = floor(j * ns.z);
  vec4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)

  vec4 x = x_ *ns.x + ns.yyyy;
  vec4 y = y_ *ns.x + ns.yyyy;
  vec4 h = 1.0 - abs(x) - abs(y);

  vec4 b0 = vec4( x.xy, y.xy );
  vec4 b1 = vec4( x.zw, y.zw );

  //vec4 s0 = vec4(lessThan(b0,0.0))*2.0 - 1.0;
  //vec4 s1 = vec4(lessThan(b1,0.0))*2.0 - 1.0;
  vec4 s0 = floor(b0)*2.0 + 1.0;
  vec4 s1 = floor(b1)*2.0 + 1.0;
  vec4 sh = -step(h, vec4(0.0));

  vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
  vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

  vec3 p0 = vec3(a0.xy,h.x);
  vec3 p1 = vec3(a0.zw,h.y);
  vec3 p2 = vec3(a1.xy,h.z);
  vec3 p3 = vec3(a1.zw,h.w);

//Normalise gradients
  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;

// Mix final noise value
  vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
  vec4 m2 = m * m;
  vec4 m4 = m2 * m2;
  vec4 pdotx = vec4(dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3));

// Determine noise gradient
  vec4 temp = m2 * m * pdotx;
  gradient = -8.0 * (temp.x * x0 + temp.y * x1 + temp.z * x2 + temp.w * x3);
  gradient += m4.x * p0 + m4.y * p1 + m4.z * p2 + m4.w * p3;
  gradient *= 42.0;

  return 42.0 * dot(m4, pdotx);
}

)","");

	//TODO multiply seed by "i"
	std::string code;
	switch (params.mode) {
		case DEFAULT: {
			code = R"(
	vec3 p = scale*spheric_to_cartesian(tex_to_spheric(st));

	float current_amplitude = 1;
	float total_amplitude = 0;

	vec3 tmp;



	snoise(p,tmp);
	//vec3 a = tmp;
	//snoise(p,tmp);
	//tmp = a*0.7+tmp*0.3;

	//tmp = vec3(snoise(p+5,a),snoise(p+8,a),snoise(p+15,a))*0.5+0.5;

	//tmp = vec3(1,0,0);
	tmp = tmp-dot(tmp,p)/length(p) * p/length(p);

	vec3 u = p+tmp;
	u = u/dot(u,u);

	float theta = warp_factor*0.1*length(tmp);
	p = rotation_matrix(theta,u)*p;

	fc = 0;
	for(int i=0; i<octaves; i++) {
		fc += snoise(p+seed_offset*(i+1),tmp)*current_amplitude;  //  (1+0.4*length(snoise(p/scale,tmp)))
		p *= lacunarity;
		total_amplitude += current_amplitude;
		current_amplitude *= persistence;
	}
	fc /= total_amplitude;
	fc = (fc+1)*0.5*(higher_limit-lower_limit)+lower_limit;
)";
			break;
		}
		case RIDGED:
			code = R"(
	vec2 geo = tex_to_spheric(st);
	vec3 p = scale*vec3(sin(M_PI/2-geo.y)*cos(geo.x),sin(M_PI/2-geo.y)*sin(geo.x),cos(M_PI/2-geo.y));

	float current_amplitude = 1;
	float total_amplitude = 0;

	vec3 tmp;
	fc = 0;
	for(int i=0; i<octaves; i++) {
		fc += (1-abs(snoise(p+seed_offset,tmp)))*current_amplitude;
		p *= lacunarity;
		total_amplitude += current_amplitude;
		current_amplitude *= persistence;
	}
	fc /= total_amplitude;
	fc = (fc)*(higher_limit-lower_limit)+lower_limit;
)";
			break;
		case BILLOWY:
			code = R"(
	vec2 geo = tex_to_spheric(st);
	vec3 p = scale*vec3(sin(M_PI/2-geo.y)*cos(geo.x),sin(M_PI/2-geo.y)*sin(geo.x),cos(M_PI/2-geo.y));

	float current_amplitude = 1;
	float total_amplitude = 0;

	vec3 tmp;
	fc = 0;
	for(int i=0; i<octaves; i++) {
		fc += abs(snoise(p+seed_offset,tmp))*current_amplitude;
		p *= lacunarity;
		total_amplitude += current_amplitude;
		current_amplitude *= persistence;
	}
	fc /= total_amplitude;
	fc = (fc)*(higher_limit-lower_limit)+lower_limit;
)";
			break;
		case IQNoise:
			code = R"(
	vec2 geo = tex_to_spheric(st);
	vec3 p = scale*vec3(sin(M_PI/2-geo.y)*cos(geo.x),sin(M_PI/2-geo.y)*sin(geo.x),cos(M_PI/2-geo.y));

	float current_amplitude = 1;
	float total_amplitude = 0;

	vec3 tmp;
	vec3 dsum = vec3(0,0,0);
	fc = 0;
	for(int i=0; i<octaves; i++) {
		float n = snoise(p+seed_offset,tmp)*current_amplitude;
		vec3 radial = dot(tmp,p)/length(p) * p/length(p);
		dsum += (tmp-radial);
		fc += n/(1.0+dot(dsum,dsum));
		p *= lacunarity;
		total_amplitude += current_amplitude/(1.0+dot(dsum,dsum));
		current_amplitude *= persistence;
	}
	fc /= total_amplitude;
	fc = (fc+1)*0.5*(higher_limit-lower_limit)+lower_limit;
)";
			break;
		case SWISS:
			code = R"(
	vec2 geo = tex_to_spheric(st);
	vec3 p = vec3(sin(M_PI/2-geo.y)*cos(geo.x),sin(M_PI/2-geo.y)*sin(geo.x),cos(M_PI/2-geo.y));
	float freq = scale;

	float current_amplitude = 1;
	float total_amplitude = 0;

	vec3 tmp;

	//domain warp
	snoise(p,tmp);
	tmp = tmp-dot(tmp,p)/length(p) * p/length(p);
	vec3 u = p+tmp;
	u = u/dot(u,u);
	float theta = warp_factor*0.1*length(tmp);
	p = rotation_matrix(theta,u)*p;



	vec3 dsum = vec3(0,0,0);
	fc = 0;
	for(int i=0; i<octaves; i++) {

		vec3 u = p + cross(p,dsum);
		u = u/dot(u,u);
		float theta = 2*0.1*length(dsum);
		vec3 p_ = rotation_matrix(theta,u)*p;

		float n = snoise(freq*p_+seed_offset,tmp);
//float n = snoise(freq*(p + 0.15*dsum)+seed_offset,tmp);
		vec3 radial = dot(tmp,p)/length(p) * p/length(p);
		dsum += (tmp-radial)*(-n)*current_amplitude;
		fc += (1-abs(n))*current_amplitude;
		freq *= lacunarity;
		//p *= lacunarity;
		total_amplitude += current_amplitude;
		current_amplitude *= persistence*smoothstep(-1,1,fc*fc);
	}
	fc /= total_amplitude;
	fc = (fc)*(higher_limit-lower_limit)+lower_limit;
)";
			break;

		case Jordan:
			code = R"(
	vec2 geo = tex_to_spheric(st);
	vec3 p = vec3(sin(M_PI/2-geo.y)*cos(geo.x),sin(M_PI/2-geo.y)*sin(geo.x),cos(M_PI/2-geo.y));
	float freq = scale;

	float current_amplitude = 1.0;
	float total_amplitude = current_amplitude;

	vec3 tmp;

	//domain warp
	snoise(p+seed_offset,tmp);
	tmp = tmp-dot(tmp,p)/length(p) * p/length(p);
	vec3 u = p+tmp;
	u = u/dot(u,u);
	float theta = warp_factor*0.1*length(tmp);
	p = rotation_matrix(theta,u)*p;


	float n = snoise(freq*p+seed_offset,tmp);
	fc = n*n * current_amplitude;
	tmp = tmp*n;
	vec3 dsum_warp = 0.4 * (tmp -dot(tmp,p)/length(p) * p/length(p));
	vec3 dsum_damp = 1.0 * (tmp -dot(tmp,p)/length(p) * p/length(p));
	float damped_amp = current_amplitude*persistence;

	vec3 dsum = vec3(0,0,0);

	for(int i=1; i<octaves; i++) {

		vec3 u = p + cross(p,dsum_warp);
		u = u/dot(u,u);
		float theta = 2*0.1*length(dsum_warp);
		vec3 p_ = rotation_matrix(theta,u)*p;

		float n = snoise(freq*p_+seed_offset,tmp);
		fc += damped_amp * n*n;
		tmp = tmp*n;
		dsum_warp += 0.35*(tmp -dot(tmp,p)/length(p) * p/length(p));
		dsum_damp += 0.8 *(tmp -dot(tmp,p)/length(p) * p/length(p));

		freq *= lacunarity;

		total_amplitude += current_amplitude;
		current_amplitude *= persistence;
		damped_amp = current_amplitude * (1 - 1.0/(1+dot(dsum_damp,dsum_damp)));
	}
	fc /= total_amplitude;
	fc = (fc)*(higher_limit-lower_limit)+lower_limit;
)";
			break;
		case Plateaus:
			code = R"(
	vec2 geo = tex_to_spheric(st);
	vec3 p = vec3(sin(M_PI/2-geo.y)*cos(geo.x),sin(M_PI/2-geo.y)*sin(geo.x),cos(M_PI/2-geo.y));
	float freq = scale;

	float current_amplitude = 1;
	float total_amplitude = 0;

	vec3 tmp;

	//domain warp
	snoise(p,tmp);
	tmp = tmp-dot(tmp,p)/length(p) * p/length(p);
	vec3 u = p+tmp;
	u = u/dot(u,u);
	float theta = warp_factor*0.1*length(tmp);
	p = rotation_matrix(theta,u)*p;



	vec3 dsum = vec3(0,0,0);
	fc = 0;
	for(int i=0; i<octaves; i++) {



		float n = snoise(freq*p+seed_offset*(i+1),tmp);


		vec3 radial = dot(tmp,p)/length(p) * p/length(p);
		dsum =  (tmp-radial)*(1-abs(n))*n*2;

		vec3 u = p + cross(p,dsum);
		u = u/dot(u,u);
		float theta = 2*0.1*length(dsum);
		vec3 p_ = rotation_matrix(theta,u)*p;

		float r_offset = length(radial)*0.1*sign(n);
		r_offset = sign(r_offset)*sin(pow(abs(r_offset)*sqrt(M_PI)-sqrt(M_PI),2));

		n = snoise(freq*p_+seed_offset*(i+1),tmp) ;

		fc += n*current_amplitude/(1+abs(fc)*abs(fc)*5);
		freq *= lacunarity;
		total_amplitude += current_amplitude;
		current_amplitude *= persistence;
	}
	fc /= total_amplitude;
	fc = (fc+1)*0.5*(higher_limit-lower_limit)+lower_limit;
)";
			break;
	}



	Shader* shader = Shader::builder()
			.include(noise)
			.include(blendmode)
			.include(rotation_matrix)
			.create(R"(
	uniform float scale;
	uniform float persistence = 0.5;
	uniform float lacunarity = 2;
	uniform int octaves = 8;
	uniform vec3 seed_offset;
	uniform float lower_limit;
	uniform float higher_limit;
	uniform float warp_factor;
)",code + R"(
	fc = blend_mode(texture2D(img,st).r, fc, texture2D(sel,st).r);
)");



	program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();

	program->bind();
	p->setCanvasUniforms(program);

	int id = glGetUniformLocation(program->getId(), "scale");
	glUniform1f(id,params.scale);

	id = glGetUniformLocation(program->getId(), "octaves");
	glUniform1i(id,params.octaves);

	id = glGetUniformLocation(program->getId(), "lacunarity");
	glUniform1f(id,params.lacunarity);

	id = glGetUniformLocation(program->getId(), "persistence");
	glUniform1f(id,params.persistence);

	id = glGetUniformLocation(program->getId(), "warp_factor");
	glUniform1f(id,params.warp);

	id = glGetUniformLocation(program->getId(), "lower_limit");
	glUniform1f(id,params.min);

	id = glGetUniformLocation(program->getId(), "higher_limit");
	glUniform1f(id,params.max);



	std::mt19937 engine{(uint)params.seed};
	std::uniform_real_distribution<float> dis(0.0, 10000.0);
	glm::vec3 offset = glm::vec3(dis(engine),dis(engine),dis(engine));
	id = glGetUniformLocation(program->getId(), "seed_offset");
	glUniform3fv(id,1,glm::value_ptr(offset));


	p->apply(program, p->get_scratch1());
	p->get_scratch1()->swap(p->get_terrain());

	return {true,1.0};
}
