//
// Created by kuhlwein on 6/29/21.
//

#include <iostream>
#include "Project.h"
#include "Temperature.h"



void TemperatureMenu::update_self(Project *p) {

}

std::shared_ptr<BackupFilter> TemperatureMenu::makeFilter(Project *p) {
	return std::make_shared<ProgressFilter>(p, [](Project* p){return p->get_terrain();}, std::move(std::make_unique<Temperature>(p)));
}

TemperatureMenu::TemperatureMenu() : FilterModal("Temperature") {

}

Temperature::~Temperature() {

}

Temperature::Temperature(Project *p) {
	p->get_terrain()->swap(p->get_scratch1());

	Shader* shader = Shader::builder()
			.include(fragmentBase)
			.include(pidShader)
			.create("",R"(
	fc =  50;
)");
	ShaderProgram* setzero = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();

	setzero->bind();
	p->setCanvasUniforms(setzero);

	p->apply(setzero,p->get_terrain());
}

void Temperature::run() {
	for (int i=0;i<500000;i++) {
	dispatchGPU([&i](Project *p) {


		Shader *shader = Shader::builder()
				.include(fragmentBase)
				.include(cornerCoords)
				.include(texturespace_gradient)
				.create(R"(
	uniform float M=0;

	float eccentricity = 0.017;
	float gamma = 23.44/180.0*M_PI;
	float omega = 0;
	float omega2 = 77.05/180.0*M_PI;
	float dmean = 1.0;

	float S0 = 1365;

	float Q = 400;

	float S(float A) {
		return S0*(1+2*eccentricity*cos(A-omega));
	}

	float A(float M) {
		return M + (2*eccentricity-pow(eccentricity,3)/4*sin(M) + 5.0/4*pow(eccentricity,2)*sin(2*M) + 13.0/12*pow(eccentricity,3)*sin(3*M));
	}

	float Ls(float A) {
		return A - omega2;
	}

	float delta(float Ls) {
		return asin(sin(gamma)*sin(Ls));
	}

	float h0(float phi, float delta) {
		float h = sign(phi)==sign(delta) ? M_PI : 0.0;
		if(abs(phi)<=M_PI/2-abs(delta)) h = acos(-tan(phi)*tan(delta));
		return h;
	}

	float QDay(float phi, float M) {
		float delt = delta(Ls(A(M)));
		float h = h0(phi,delt);
		return S(A(M))/M_PI * (h*sin(phi)*sin(delt)+cos(phi)*cos(delt)*sin(h));
	}

)", R"(
	float T = texture(img,st).r;
	float terrain = texture(scratch1,st).r;

	float alpha = terrain>0 ? 0.15 : 0.06;
    alpha = 0.30;










	float phi = tex_to_spheric(st).y;




    //alpha = 0.354 + 0.12*0.5*(3*pow(sin(phi),2)-1);



	float ASR = (1-alpha)*(QDay(phi,M)  );//+QDay(phi,M+M_PI))/2;
	float OLR = 210 + 2 * T;

    OLR = 210*pow(T+273.15,4)/pow(273.4,4) * 0.93;

	vec2 gradient = get_texture_laplacian(st);
	float change = ASR - OLR + 0.55*1e6*(gradient.x + gradient.y);


    float atmosphere = 1e7;
	float C = atmosphere + (terrain>0 ? atmosphere*0.5 : 4*1.5 * atmosphere);
	fc =  T + change*3.154*1e7/15000/C;
)");
		ShaderProgram *mainfilter = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
				.link();

		for (int j=0; j<10; j++) {


            mainfilter->bind();
            p->setCanvasUniforms(mainfilter);
            int id = glGetUniformLocation(mainfilter->getId(), "M");
            glUniform1f(id, M_PI * 2 / 15000 * i);
            p->apply(mainfilter, p->get_scratch2());
            p->get_terrain()->swap(p->get_scratch2());
            i++;
        }

		//


	});
	}

	dispatchGPU([](Project* p){
		std::cout << "finished\n;";
		p->get_terrain()->swap(p->get_scratch2());
	});
}
