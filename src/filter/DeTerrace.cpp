//
// Created by kuhlwein on 9/21/20.
//

#include <algorithm>
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

	/*
	 * scratch 2 -> pid
	 */
	Shader* shader = Shader::builder()
			.include(fragmentBase)
			.include(pidShader)
			.create("",R"(
	fc =  uintBitsToFloat(coordToPid(st,img));
)");
	init1 = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();


	/*
	 * scratch 2 -> if neightbour: neighbour else self
	 */

	shader = Shader::builder()
			.include(fragmentBase)
			.include(cornerCoords)
			.include(pidShader)
			.create(R"(
uniform vec2 primary_offset;
)",R"(
	vec2 resolution = textureSize(img,0);
	vec2 o = primary_offset;
	float a;
	if (texture(img,offset(st,o,resolution)).r == texture(img,st).r) {
		a = texture(scratch2,st).r;
	} else {
		a = texture(scratch2,offset(st,o,resolution)).r;
	}
	fc = a;
)");
	init2 = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
			.link();





	/*
	 * if same height:
	 * 		check distance, but only if not self.
	 * if different height (Dont do this, do a pre-scan)
	 * 		check distance
	 */

	Shader* fragment_set = Shader::builder()
			.include(fragmentBase)
			.include(cornerCoords)
			.include(distance)
			.include(pidShader)
			.create(R"(
uniform vec2 primary_offset;
uniform vec2 secondary_offset;
)",R"(
	vec2 resolution = textureSize(img,0);

	float min_d = -1.0;
	uint pid = floatBitsToUint(texture(scratch2,st).r);

	if (coordToPid(st,scratch2) != pid) {
		min_d = geodistance(st,pidToCoord(pid,img),resolution);
	}

	float d;
	uint newPid;
	vec2 o;

	o = secondary_offset;
	newPid = floatBitsToUint(texture(scratch2,offset(st,o,resolution)).r);
	d = texture(img,pidToCoord(newPid,img)).r;

	if (d != texture(img,st).r && floatBitsToUint(texture(scratch2,offset(st,o,resolution)).r)!=coordToPid(offset(st,o,resolution),img) && (min_d<0 || geodistance(st,pidToCoord(newPid,img),resolution)<min_d)) {
		min_d = geodistance(st,pidToCoord(newPid,img),resolution);
		pid = newPid;
	}

	o = primary_offset;
	newPid = floatBitsToUint(texture(scratch2,offset(st,o,resolution)).r);
	d = texture(img,pidToCoord(newPid,img)).r;

	if (d != texture(img,st).r && floatBitsToUint(texture(scratch2,offset(st,o,resolution)).r)!=coordToPid(offset(st,o,resolution),img) && (min_d<0 || geodistance(st,pidToCoord(newPid,img),resolution)<min_d)) {
		min_d = geodistance(st,pidToCoord(newPid,img),resolution);
		pid = newPid;
	}

	fc = uintBitsToFloat(pid);
)");
	program = ShaderProgram::builder()
			.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
			.addShader(fragment_set->getCode(), GL_FRAGMENT_SHADER)
			.link();
}

int intFromFloat(float* f, int index) {
	int* i = (int*) f;
	return i[index];
}

float DeTerrace::calc() {

}

void DeTerrace::run() {
	std::unique_ptr<float[]> rightdown = get(glm::vec2(1,1),glm::vec2(1,0));
	std::unique_ptr<float[]> downright = get(glm::vec2(0,1),glm::vec2(1,1));
	std::unique_ptr<float[]> downleft = get(glm::vec2(-1,1),glm::vec2(0,1));
	std::unique_ptr<float[]> leftdown = get(glm::vec2(-1,0),glm::vec2(-1,1));
	std::unique_ptr<float[]> leftup = get(glm::vec2(-1,-1),glm::vec2(-1,0));
	std::unique_ptr<float[]> upleft = get(glm::vec2(0,-1),glm::vec2(-1,-1));
	std::unique_ptr<float[]> upright = get(glm::vec2(1,-1),glm::vec2(0,-1));
	std::unique_ptr<float[]> rightup = get(glm::vec2(1,0),glm::vec2(1,-1));

	std::unique_ptr<float[]> height;
	dispatchGPU([this,&height](Project* p) {
		height = p->get_terrain()->downloadDataRAW();
	});

	auto pidToCoord = [this](int pid) {
		uint a = pid - p->getWidth()*(pid/p->getWidth());
		uint b = (pid - a)/p->getWidth();
		return glm::ivec2(a,b);
	};

	//TODO wrapping?
	auto tovec = [this,&height,&pidToCoord](int i, int id, float minheight) {
		auto xy = pidToCoord(i)-pidToCoord(id);
		return glm::vec3(xy.x,xy.y,std::max(height[id],minheight));
	};



	data = std::make_unique<float[]>(p->getHeight()*p->getWidth());

//	for (int i=0; i<p->getHeight()*p->getWidth(); i++) {
//		data[i] = float(((int*)leftup.get())[i]);
//	};
//	dispatchGPU([this](Project* p) {
//		p->get_terrain()->uploadData(GL_RED, GL_FLOAT, data.get());
//	});
//	setProgress({true,1});
//	return;

	for (int i=0; i<p->getHeight()*p->getWidth(); i++) {
		glm::vec3 p[16];
		int index=0;
		for (auto d : {rightdown.get(), downright.get(), downleft.get(), leftdown.get(), leftup.get(), upleft.get(), upright.get(), rightup.get()}) {
			int lu = intFromFloat(d,i);
			int lu2 = intFromFloat(d,lu);
			p[index++] = tovec(i,lu,height[i]);
			p[index++] = tovec(i,lu2,height[lu]);
		}

		//START
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

//		std::cout << "vec: \n";
//		for (int i=0; i<16; i++) std::cout << p[i].x << " " << p[i].y << " " << p[i].z << "\n";
//		for (int i=0; i<19; i++) {
//			std::cout << "[";
//			for(int j=0; j<18; j++) std::cout << A[i*19+j] << ",";
//			std::cout << A[i*19+18] << "],";
//		}
//		std::cout << "\n";

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

//		if (std::isnan(val)) {
//			std::cout << i  << " " << val << "\n";
//			for(auto pp : p) {std::cout << pp.x << " " << pp.y << " " << pp.z << ", ";}
//			std::cout << "\n";
//			for (int i=0; i<16; i++) std::cout <<  x[i]<< " "; std::cout << "\n";
//			std::cout << "\n";
//			for (int i=0; i<19; i++) {
//				for(int j=0; j<19; j++) std::cout << A[i*19+j] << " "; std::cout << "\n";
//			}
//			std::cout << "\n";
//		}


		data[i] = val;
	}

	dispatchGPU([this](Project* p) {
		p->get_terrain()->uploadData(GL_RED, GL_FLOAT, data.get());
	});
	dispatchGPU([this](Project* p) {
		Shader* fragment_set = Shader::builder()
				.include(fragmentBase)
				.include(cornerCoords)
				.include(distance)
				.include(pidShader)
				.create("",R"(
	vec2 resolution = textureSize(img,0);

	uint pid = floatBitsToUint(texture(scratch2,st).r);

	fc = float(pid);
	if (pid == coordToPid(st,img)) fc = -1;
	fc = geodistance(st,pidToCoord(pid,img),resolution);
	)");
		program = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(fragment_set->getCode(), GL_FRAGMENT_SHADER)
				.link();
		program->bind();
		p->setCanvasUniforms(program);
		p->apply(program, p->get_scratch1());
		p->get_scratch1()->swap(p->get_scratch2());


		//target->swap(p->get_scratch2());

	});
	setProgress({true,1});
}


std::unique_ptr<float[]> DeTerrace::get(glm::vec2 primary, glm::vec2 secondary) {
	std::unique_ptr<float[]> data;

	dispatchGPU([this, primary, secondary, &data](Project* p) {
		init1->bind();
		p->setCanvasUniforms(init1);

		p->apply(init1,p->get_scratch2());

		init2->bind();
		p->setCanvasUniforms(init2);
		int id = glGetUniformLocation(program->getId(), "primary_offset");
		glUniform2f(id,primary.x,primary.y);
		id = glGetUniformLocation(program->getId(), "offset_no_globe_wrap");
		glUniform1i(id,true);
		p->apply(init2,p->get_scratch1());
		p->get_scratch1()->swap(p->get_scratch2());

		int a = std::ceil(log2(std::max(p->getHeight(),p->getWidth())))-3;

		for (int i=0; i<=a; i++) {
			program->bind();
			float r = pow(2,i);
			id = glGetUniformLocation(program->getId(), "primary_offset");
			glUniform2f(id,r*primary.x,r*primary.y);
			id = glGetUniformLocation(program->getId(), "secondary_offset");
			glUniform2f(id,r*secondary.x,r*secondary.y);
			id = glGetUniformLocation(program->getId(), "offset_no_globe_wrap");
			glUniform1i(id,true);

			p->setCanvasUniforms(program);
			p->apply(program, p->get_scratch1());
			p->get_scratch1()->swap(p->get_scratch2());

		}
		for (int i=a; i>=0; i--) {
			program->bind();
			float r = pow(2,i);
			id = glGetUniformLocation(program->getId(), "primary_offset");
			glUniform2f(id,r*primary.x,r*primary.y);
			id = glGetUniformLocation(program->getId(), "secondary_offset");
			glUniform2f(id,r*secondary.x,r*secondary.y);
			id = glGetUniformLocation(program->getId(), "offset_no_globe_wrap");
			glUniform1i(id,true);

			p->apply(program, p->get_scratch1());
			p->get_scratch1()->swap(p->get_scratch2());
		}

		data = (p->get_scratch2()->downloadDataRAW()); //TODO better solution than casting
	});
	return std::move(data);
}