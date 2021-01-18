//
// Created by kuhlwein on 9/21/20.
//

#include "Project.h"
#include "DeTerrace.h"
#include "BlurMenu.h"

DeTerraceMenu::DeTerraceMenu() : FilterModal("Deterrace") {

}

void DeTerraceMenu::update_self(Project *p) {

}

std::shared_ptr<BackupFilter> DeTerraceMenu::makeFilter(Project *p) {
	return std::make_shared<ProgressFilter>(p, [](Project* p){return p->get_terrain();}, std::move(std::make_unique<DeTerrace>(p, p->get_terrain())));
}

DeTerrace::DeTerrace(Project *p, Texture *target) {
	this->p = p;
	this->target = target;

	dim = 512;

	tex = new Texture(dim,dim,GL_R32F,"mini_tmp",GL_NEAREST);


	Shader* offset_shader = Shader::builder()
			.create(R"(
uniform float tmp_dim;
uniform vec2 tmp_offset;
)","");


	Shader* shader = Shader::builder()
			.include(fragmentBase)
			.include(cornerCoords)
			.include(offset_shader)
			.create(R"(
	void find_contour_edges(vec2 p, float phi_offset, vec2 resolution, out vec3 p1, out vec3 p2) {
		int N = 100;

		float epsilon = 1e-4;

		p1 = vec3(0,0,texture2D(img, p).r);
		for (int i=1; i<N; i++) {
			for(int phi=0; phi<16; phi++) {
				float a = 3.141592/2/2*phi/15 + phi_offset;
				vec2 dp = vec2(i*cos(a)+cos(phi_offset),i*sin(a)+sin(phi_offset));
				float h = texture2D(img, offset(p,dp,resolution)).r;
				if(abs(h-p1.z) > epsilon && p1.xy==vec2(0)) {
					p1=vec3(dp.x,dp.y,max(h,p1.z));
				}
			}
		}
		for (int i=0; i<N; i++) {
			float ii = (N*pow(i,1.5));
			for(int phi=0; phi<16; phi++) {
				float a = 3.141592/2/2*phi/15 + phi_offset;
				vec2 dp = vec2(ii*cos(a)+cos(phi_offset),ii*sin(a)+sin(phi_offset));
				float h = texture2D(img, offset(p, dp,resolution)).r;
				if(abs(h-p1.z) > epsilon && p1.xy==vec2(0)) {
					p1=vec3(dp.x,dp.y,max(h,p1.z));
				}
			}
		}
		if(p1.xy==vec2(0)) p1.xy=(N+pow(N,1.5))*vec2(cos(phi_offset),sin(phi_offset));

		vec2 dp = normalize(p1.xy);
		p2 = vec3(0,0,p1.z);
		for (int i=1; i<N; i++) {
			float h = texture2D(img, offset(p, i*dp+p1.xy,resolution)).r;
			if(abs(h-p2.z) > epsilon && p2.xy==vec2(0)) {
				p2=vec3((i-0.5)*dp.x+p1.x,(i-0.5)*dp.y+p1.y,max(h,p2.z));
			}
		}
		for (int i=1; i<N; i++) {
			float ii = (N+pow(i,1.5));
			float h = texture2D(img, offset(p, ii*dp,resolution)).r;
			if(abs(h-p2.z) > epsilon && p2.xy==vec2(0)) {
				p2=vec3(ii*dp.x+p1.x,ii*dp.y+p1.y,max(h,p2.z));
			}
		}
		if(p2.xy==vec2(0)) p2.xy=(N+pow(N,1.5))*dp;

		//fix local hills
		if(abs(p1.z-p2.z)<epsilon) {
			p2.xy = (p1.xy+p2.xy);
			p2.z = p2.z + p1.z-texture2D(img, p).r;
		}

		//Fix too strong extrapolation
//		float slope = (p2.z-p1.z)/length(p2.xy-p1.xy);
//		float prediction = p1.z-slope*length(p1.xy);
//		if(prediction<texture2D(img, p).r) p2.xy = p2.xy*(p2.z-p1.z)/(p1.z/2-texture2D(img,p).r/2)*length(p1.xy)+length(p1.xy);

	}

)",R"(
	vec2 resolution = textureSize(img,0);

	vec2 st_ = (st*tmp_dim+tmp_offset)/resolution;

	vec3 p1;
	vec3 p2;
	vec3 p[16];
	int counter = 0;
	for(int dx=-1; dx<=1; dx++) for(int dy=-1; dy<=1; dy++) {
		if(dx==0 && dy==0) continue;
		find_contour_edges(st_,atan(dx,dy),resolution,p1,p2);
		p[counter]=p1; p[counter+1]=p2;
		counter+=2;
	}

	float epsilon = 1e-6;

	float A[19*19];
	for (int x=0; x<19; x++) for (int y=0; y<19; y++) {
		float val = 0;
		if (x<16 && y<16) val=(pow(p[x].x-p[y].x,2)+pow(p[x].y-p[y].y,2))*log(pow(p[x].x-p[y].x,2)+pow(p[x].y-p[y].y,2)+epsilon);
		if (x==16 || y==16) val=1.0;
		if (x==17) val=p[y].x;
		if (y==17) val=p[x].x;
		if (x==18) val=p[y].y;
		if (y==18) val=p[x].y;
		if (x==y) val=0;
		if (y>=16 && x>=16) val=0;
		A[x*19+y] = val;
	}

	//invert A
	const int N = 19;
	int P[N+1];
	for (int i=0; i<=N; i++) P[i] = i;

    for (int i=0; i<N; i++) {
        float maxA = 0.0;
        int imax = i;

        for (int k=i; k<N; k++) {
            if (abs(A[k*19+i]) > maxA) {
                maxA = abs(A[k*19+i]);
                imax = k;
            }
		}

        if (imax != i) {
            int j = P[i];
            P[i] = P[imax];
            P[imax] = j;

			for(int k=0; k<N; k++) {
				float tmp = A[i*19+k];
            	A[i*19+k] = A[imax*19+k];
            	A[imax*19+k] = tmp;
			}

            P[N]++;
        }

        for (int j=i+1; j<N; j++) {
            A[j*19+i] /= A[i*19+i];

            for (int k=i+1; k<N; k++)
                A[j*19+k] -= A[j*19+i]*A[i*19+k];
        }
    }

	float b[19];
	for(int i=0; i<N; i++) {
		b[i]=0;
		if(i<16) b[i] = p[i].z;
	}

	float x[19];
	for (int i=0; i<N; i++) {
        x[i] = b[P[i]];

        for (int k = 0; k < i; k++)
            x[i] -= A[i*19+k] * x[k];
    }

    for (int i=N-1; i>=0; i--) {
        for (int k=i+1; k<N; k++)
            x[i] -= A[i*19+k] * x[k];

        x[i] /= A[i*19+i];
    }

	float val = x[16];
	for (int i=0; i<16; i++) val+= x[i]*(pow(p[i].x,2)+pow(p[i].y,2))*log(pow(p[i].x,2)+pow(p[i].y,2)+epsilon);

	float DD = 0;
	for (int i=0; i<16; i++) DD+= abs(p[i].x)+abs(p[i].y);

	float oldval = texture2D(img, st_).r;
//	val = max(val,oldval);
//	for(int i=0; i<16; i++) {
//		if(p[i].z>oldval+epsilon) val = min(val,p[i].z);
//	}

	for(int i=0; i<16; i++) {
		if(p[i].z>oldval+epsilon && val>p[i].z) val = 2;
	}

	if(val<oldval) val=-2;

	fc = val;
)");

	program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();



	Shader* copy_shader = Shader::builder()
			.include(fragmentBase)
			.include(offset_shader)
			.create("uniform sampler2D mini_tmp;",R"(
fc = texture(scratch1,st).r;

vec2 resolution = textureSize(img,0);

vec2 st_ = (st*resolution-tmp_offset)/tmp_dim;

if(all(lessThan(st_,vec2(1.0))) && all(greaterThan(st_,vec2(0.0)))) {
	fc = texture(mini_tmp, st_).r;
}
)");
	copy = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(copy_shader->getCode(),GL_FRAGMENT_SHADER)
			.link();
}

std::pair<bool, float> DeTerrace::step(Project *p) {
	int xi = ceil((double)(p->getWidth())/dim);
	int yi = ceil((double)(p->getHeight())/dim);

	if (i<=xi*yi) {
		int xoffset, yoffset;
		xoffset = dim*(i%xi);
		yoffset = dim*(i/xi);

		program->bind();
		int id = glGetUniformLocation(program->getId(), "tmp_offset");
		glUniform2f(id,xoffset, yoffset);
		id = glGetUniformLocation(program->getId(), "tmp_dim");
		glUniform1f(id,dim);
		p->setCanvasUniforms(program);

		p->apply(program, tex);





		p->add_texture(tex);


		copy->bind();
		id = glGetUniformLocation(copy->getId(), "tmp_offset");
		glUniform2f(id,xoffset, yoffset);
		id = glGetUniformLocation(copy->getId(), "tmp_dim");
		glUniform1f(id,dim);
		p->setCanvasUniforms(copy);
		p->apply(copy,p->get_scratch2());

		p->remove_texture(tex);

		p->get_scratch2()->swap(p->get_scratch1());

		i++;
		return {false, (float)(i)/xi/yi};
	}



	program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(copy_img->getCode(), GL_FRAGMENT_SHADER)
			.link();
	program->bind();
	p->setCanvasUniforms(program);
	p->apply(copy,target,{{p->get_scratch1(),"to_be_copied"}});




	return {true,i};
}
