//
// Created by kuhlwein on 6/29/21.
//

#include <iostream>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "Project.h"
#include "Tectonics.h"



void TectonicsMenu::update_self(Project *p) {

}

std::shared_ptr<BackupFilter> TectonicsMenu::makeFilter(Project *p) {
	return std::make_shared<ProgressFilter>(p, [](Project* p){return p->get_terrain();}, std::move(std::make_unique<Tectonics>(p)));
}

TectonicsMenu::TectonicsMenu() : FilterModal("Tectonics") {

}

Tectonics::~Tectonics() {

}

Tectonics::Tectonics(Project *p) {
    plates.push_back(new Plate(p->getWidth(),p->getHeight()));
    plates.push_back(new Plate(p->getWidth(),p->getHeight()));
    plates[0]->updateRotationBy((float)0.01, glm::vec3(1, 0, 0));
    plates[1]->updateRotationBy(-(float)0.01, glm::vec3(0, 0, 1));

    a = new Texture(p->getWidth(),p->getWidth(),GL_RGBA32F,"");
    b = new Texture(p->getWidth(),p->getWidth(),GL_RGBA32F,"");



	Shader* shader = Shader::builder()
			.include(fragmentColor)
			.create("",R"(
	fc =  st.x>0.5 ? vec4(1.0) : vec4(0);
)");
	setzero = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();

	setzero->bind();
	p->setCanvasUniforms(setzero);
	p->apply(setzero,plates[0]->getTexture());

	shader = Shader::builder()
            .include(fragmentColor)
            .create("",R"(
	fc =  st.x<0.5 ? vec4(1.0) : vec4(0);
)");
    setzero = ShaderProgram::builder()
            .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
            .addShader(shader->getCode(), GL_FRAGMENT_SHADER)
            .link();

    setzero->bind();
    p->setCanvasUniforms(setzero);
    p->apply(setzero,plates[1]->getTexture());


    tectonicSamplingShader = Shader::builder()
            .create(R"(
uniform sampler2D plate;

uniform mat3 rotationmatrix;
uniform mat3 inverserotationmatrix;
uniform float plateIndex;
uniform vec3 plateVelocity;

vec4 plateTexture(sampler2D img, vec2 st) {
    vec2 spheric = tex_to_spheric(st);
    vec3 cartesian = spheric_to_cartesian(spheric);

    cartesian = rotationmatrix*cartesian;

    spheric = cartesian_to_spheric(cartesian);
    vec2 st_ = spheric_to_tex(spheric);

	return texture(img,st_);
}

vec4 inverseplateTexture(sampler2D img, vec2 st) {
    vec2 spheric = tex_to_spheric(st);
    vec3 cartesian = spheric_to_cartesian(spheric);

    cartesian = inverserotationmatrix*cartesian;

    spheric = cartesian_to_spheric(cartesian);
    vec2 st_ = spheric_to_tex(spheric);

	return texture(img,st_);
}
)","");

    /*
     * SET SHADERS
     */

    shader = Shader::builder()
            .include(fragmentColor)
            .include(cornerCoords)
            .include(tectonicSamplingShader)
            .create("",R"(
	fc = vec4(0.0);
)");
    setzero = ShaderProgram::builder()
            .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
            .addShader(shader->getCode(), GL_FRAGMENT_SHADER)
            .link();

    shader = Shader::builder()
            .include(fragmentColor)
            .include(cornerCoords)
            .include(tectonicSamplingShader)
            .include(directional)
            .create(R"(
    uniform sampler2D foldtex;
)",R"(
    fc = texture(foldtex,st);
	float index = plateIndex;

    vec2 spheric_coord = tex_to_spheric(st);
	vec3 cartesian_coord = spheric_to_cartesian(spheric_coord);
    vec3 v = cross(cartesian_coord,plateVelocity);
    vec2 spheric_v = cartesian_to_v(v,spheric_coord);
    float theta = atan(spheric_v.y,spheric_v.x);

    if (plateTexture(plate,st).r>0) fc = vec4(index,theta,0,0);
)");
    foldShader = ShaderProgram::builder()
            .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
            .addShader(shader->getCode(), GL_FRAGMENT_SHADER)
            .link();
}

void Tectonics::run() {
    for (int i=0; i<100000; i++) {
        for (Plate* p : plates) {
            p->rotate();
        }



        dispatchGPU([this](Project* p) {

            //Sets distance to zero, sets direction of movement, and plate index, (and age???)
            fold(setzero, foldShader, a, b, p);


            //set distance for all to zero, set plate for all to plate
            //Check neighbours distance, if shorter than current or currently not part of plate, update current plate and distance
            //ONLY if direction of plate movement matches!!

            //When finished, do backwards from borders between plates


            auto shader2 = Shader::builder()
                    .include(fragmentColor)
                    .include(cornerCoords)
                    .include(distance)
                    .include(directional)
                    .create(R"(
    uniform sampler2D foldtex;
    uniform float radius;
)",R"(
	fc = texture(foldtex,st);

    vec2 resolution = textureSize(foldtex,0);
    float phi = tex_to_spheric(st).y;
    float factor = 1/cos(abs(phi));
    int N = 16;


    for (int i=0; i<N; i++) {
        vec2 neighbour = offset(st, vec2(cos(2*3.14159*i/N)*radius*factor,sin(2*3.14159*i/N)*radius),resolution);
        vec4 a = texture(foldtex, neighbour);

        //float theta = atan(sin(2*3.14159*i/N),cos(2*3.14159*i/N));

        //vec3 ndiff = spheric_to_cartesian(tex_to_spheric(neighbour)) - spheric_to_cartesian(tex_to_spheric(st));
        //vec2 v = cartesian_to_v(ndiff,tex_to_spheric(st));
        //float ntheta = atan(v.y,v.x);

        float nz = a.z+geodistance(neighbour,st,resolution);

        //&& abs(ntheta-a.y)<2*M_PI/N

        if((nz < fc.z || fc.x==0) && a.x != 0) fc = vec4(a.x, a.y, nz, a.a);
    }
)");

            ShaderProgram* foldShader2 = ShaderProgram::builder()
                    .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
                    .addShader(shader2->getCode(), GL_FRAGMENT_SHADER)
                    .link();

            int N = 5;
            for (int i=0; i<N; i++) {
                foldShader2->bind();
                p->setCanvasUniforms(foldShader2);
                int id = glGetUniformLocation(foldShader2->getId(),"radius");
                glUniform1f(id,pow(2,i));
                p->apply(foldShader2,a,{{b,"foldtex"}});
                a->swap(b);
            }
            for (int i=N; i>0; i--) {
                foldShader2->bind();
                p->setCanvasUniforms(foldShader2);
                int id = glGetUniformLocation(foldShader2->getId(),"radius");
                glUniform1f(id,pow(2,i));
                p->apply(foldShader2,a,{{b,"foldtex"}});
                a->swap(b);
            }




            /*
            // Render
            */

            auto shader = Shader::builder()
                    .include(fragmentBase)
                    .include(cornerCoords)
                    .create(R"(
    uniform sampler2D foldtex;
)",R"(
	fc = texture(foldtex,st).z;
    if(fc==0) fc = texture(foldtex,st).x;
    fc = texture(foldtex,st).x;
)");

            ShaderProgram* foldShader = ShaderProgram::builder()
                    .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
                    .addShader(shader->getCode(), GL_FRAGMENT_SHADER)
                    .link();
            foldShader->bind();

            p->setCanvasUniforms(foldShader);

            p->apply(foldShader,p->get_terrain(),{{b,"foldtex"}});

            /*
             * Back to plates
             */

            auto shader3 = Shader::builder()
                    .include(fragmentColor)
                    .include(cornerCoords)
                    .include(distance)
                    .include(directional)
                    .include(tectonicSamplingShader)
                    .create(R"(
    uniform sampler2D foldtex;
)",R"(
    vec4 a = inverseplateTexture(foldtex,st);
    fc = a.x == plateIndex ? vec4(1.0) : vec4(0.0);
    //fc = a-plateIndex;

    //fc = plateIndex==1 ? (st.x>0.5 ? vec4(1.0) : vec4(-1.0)) : (st.x<0.5 ? vec4(1.0) : vec4(-1.0));
)");

            ShaderProgram* unfoldshader = ShaderProgram::builder()
                    .addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
                    .addShader(shader3->getCode(), GL_FRAGMENT_SHADER)
                    .link();

            float index = 1; //Index 0 is no plate
            for (Plate* plate : plates) {
                //a->swap(plate->getTexture());

                unfoldshader->bind();
                p->setCanvasUniforms(unfoldshader);
                plate->setPlateUniforms(unfoldshader,index++);


                p->apply(unfoldshader,plate->getTexture(),{{b,"foldtex"}});
            }


        });
    }

    setProgress({true,1});
}

//Apply operations to t1
void Tectonics::fold(ShaderProgram *zero, ShaderProgram *operation, Texture* t1, Texture* t2, Project *p) {
    zero->bind();
    p->apply(zero,t1);
    t1->swap(t2);

    float index = 1; //Index 0 is no plate
    for (Plate* plate : plates) {

        operation->bind();
        plate->setPlateUniforms(operation,index++);
        p->setCanvasUniforms(operation);

        p->apply(operation,t1,{{t2,"foldtex"},{plate->getTexture(),"plate"}});
        t1->swap(t2);
    }
}

Plate::Plate(int width, int height) {
    texture = new Texture(width,height,GL_RGBA32F,"plate");

    rotation = glm::mat4(1);
    angularVelocity = glm::vec3(0);
}

Plate::~Plate() {
    delete texture;
}

Texture *Plate::getTexture() {
    return texture;
}

void Plate::rotate() {
    rotation = glm::rotate(rotation,glm::length(angularVelocity), glm::normalize(angularVelocity));
}

void Plate::setPlateUniforms(ShaderProgram* shaderProgram, int index) {
    int id = glGetUniformLocation(shaderProgram->getId(),"plateIndex");
    glUniform1f(id,index);

    glm::mat3 a = rotation;
    id = glGetUniformLocation(shaderProgram->getId(),"rotationmatrix");
    glUniformMatrix3fv(id,1,GL_FALSE,glm::value_ptr(a));

    glm::mat3 b = glm::transpose(a);
    id = glGetUniformLocation(shaderProgram->getId(),"inverserotationmatrix");
    glUniformMatrix3fv(id,1,GL_FALSE,glm::value_ptr(b));

    id = glGetUniformLocation(shaderProgram->getId(),"plateVelocity");
    glUniform3f(id,angularVelocity.x,angularVelocity.y,angularVelocity.z);
}

void Plate::updateRotationBy(float theta, glm::vec3 axis) {
    angularVelocity += theta*axis;
}



/*
 * plateID (PLATE elevation/age/direction)
 */



/*
 * Index, movement-direction, distance
 */
/*
 * Ocean:
 * For each pixel, get current plate
 * Spread, gaussian-like, remember distance from divergence to plates and from plate to divergence
 * For each plate generate new crust based on distance
 *
 *
 *
 *
 */