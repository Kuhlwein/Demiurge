//
// Created by kuhlwein on 8/12/20.
//

#include "Project.h"
#include <imgui/imgui.h>
#include <iostream>
#include <unordered_map>
#include "FlowFilter.h"

FlowfilterMenu::FlowfilterMenu() : FilterModal("Flowfilter") {

}

void FlowfilterMenu::update_self(Project *p) {
	ImGuiIO io = ImGui::GetIO();

	const char* items[] = { "Erode","Dilate"};
	static int current = 0;
	ImGui::Combo("Operation",&current, items,IM_ARRAYSIZE(items));
}

std::shared_ptr<BackupFilter> FlowfilterMenu::makeFilter(Project *p) {
	auto morph = std::make_unique<FlowFilter>();
	return std::make_shared<ProgressFilter>(p,[](Project* p){return p->get_terrain();},std::move(morph));
}

FlowFilter::FlowFilter() {

}

FlowFilter::~FlowFilter() {

}

std::vector<int> FlowFilter::neighbours(int pos, int dat) {
	//data-1 -> moves to the right (negative delta is left)
	//data-width -> moves down (negative delta is up)
	std::vector<int> n;
	auto f = [this,&n](int pos, int dx, int dy) {
		int x = pos%width;
		int y = pos/width;
		x = x+dx;
		if (wrapx) {
			x = (x+width)%width;
		} else {
			if(x>=width || x<0) return;
		}
		y = y+dy;
		if(y>=height || y<0) return;
		n.emplace_back(y*width+x);
	};
	if (Nthbit(dat,1)) f(pos,-1,-1);
	if (Nthbit(dat,2)) f(pos,0,-1);
	if (Nthbit(dat,3)) f(pos,1,-1);
	if (Nthbit(dat,4)) f(pos,-1,0);
	if (Nthbit(dat,6)) f(pos,1,0);
	if (Nthbit(dat,7)) f(pos,-1,1);
	if (Nthbit(dat,8)) f(pos,0,1);
	if (Nthbit(dat,9)) f(pos,1,1);
	return n;
}

void FlowFilter::run() {
	float c1;
	/*
	 * all bits zero -> not point of interest
	 *
	 * Bits set for neighbour, 5'th bit is self and indicates a sink/lake
	 * 1 2 3
	 * 4 5 6
	 * 7 8 9
	 *
	 * the 10'th bit indicates that this is a border sink/lake, aka a river mouth
	 */

//Find magic numbers
std::cout << "Finding points of interest\n";
c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;
	dispatchGPU([this](Project* p){
		wrapx = std::abs(p->getCoords()[3]-p->getCoords()[2])>2*M_PI-1e-3;
		std::cout << wrapx << " wrap\n";

		Shader* shader = Shader::builder()
				.include(fragmentBase)
				.include(cornerCoords)
				.create("",R"(
	vec2 resolution = textureSize(img,0);

	float a = texture2D(img, st).r;
	fc = 0.0;
	if (a<=0.0f) return;
	if (texture2D(sel, st).r==0) return;
	fc = 5.0;
	float a2 = a;
	//(vec(1,1)-> right/down)

	a2 = texture2D(img, offset(st, vec2(1,1),resolution)).r;
	if(a2<a) {
		fc = 9;
		a = a2;
	}
	a2 = texture2D(img, offset(st, vec2(0,1),resolution)).r;
	if(a2<a) {
		fc = 8;
		a = a2;
	}
	a2 = texture2D(img, offset(st, vec2(-1,1),resolution)).r;
	if(a2<a) {
		fc = 7;
		a = a2;
	}
	a2 = texture2D(img, offset(st, vec2(1,0),resolution)).r;
	if(a2<a) {
		fc = 6;
		a = a2;
	}
	a2 = texture2D(img, offset(st, vec2(-1,0),resolution)).r;
	if(a2<a) {
		fc = 4;
		a = a2;
	}
	a2= texture2D(img, offset(st, vec2(1,-1),resolution)).r;
	if(a2<a) {
		fc = 3;
		a = a2;
	}
	a2 = texture2D(img, offset(st, vec2(0,-1),resolution)).r;
	if(a2<a) {
		fc = 2;
		a = a2;
	}
	a2 = texture2D(img, offset(st, vec2(-1,-1),resolution)).r;
	if(a2<a) {
		fc = 1;
		a = a2;
	}
)");
		ShaderProgram* program = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
				.link();

		program->bind();
		p->setCanvasUniforms(program);

		p->apply(program,p->get_scratch1());
		//p->get_scratch1()->swap(p->get_terrain());
		delete shader;
		delete program;

		shader = Shader::builder()
				.include(fragmentBase)
				.include(cornerCoords)
				.create("",R"(
	vec2 resolution = textureSize(img,0);

	float a = texture2D(img, st).r;
	fc = -1.0;
	if (a<=0.0f) return;
	if (texture2D(sel, st).r==0) return;
	fc = 0.5;

	bool flag = false;
	//(vec(1,1)-> right/down)

	if(texture2D(scratch1, offset(st, vec2(1,1),resolution)).r==1) fc+=256;
	if(texture2D(scratch1, offset(st, vec2(0,1),resolution)).r==2) fc+=128;
	if(texture2D(scratch1, offset(st, vec2(-1,1),resolution)).r==3) fc+=64;
	if(texture2D(scratch1, offset(st, vec2(1,0),resolution)).r==4) fc+=32;
	if(texture2D(scratch1, st).r==5) fc+=16;
	if(texture2D(scratch1, offset(st, vec2(-1,0),resolution)).r==6) fc+=8;
	if(texture2D(scratch1, offset(st, vec2(1,-1),resolution)).r==7) fc+=4;
	if(texture2D(scratch1, offset(st, vec2(0,-1),resolution)).r==8) fc+=2;
	if(texture2D(scratch1, offset(st, vec2(-1,-1),resolution)).r==9) fc+=1;

	if(texture2D(scratch1, offset(st, vec2(1,1),resolution)).r<0) flag=true;
	if(texture2D(scratch1, offset(st, vec2(0,1),resolution)).r<0) flag=true;
	if(texture2D(scratch1, offset(st, vec2(-1,1),resolution)).r<0) flag=true;
	if(texture2D(scratch1, offset(st, vec2(1,0),resolution)).r<0) flag=true;
	if(texture2D(scratch1, offset(st, vec2(-1,0),resolution)).r<0) flag=true;
	if(texture2D(scratch1, offset(st, vec2(1,-1),resolution)).r<0) flag=true;
	if(texture2D(scratch1, offset(st, vec2(0,-1),resolution)).r<0) flag=true;
	if(texture2D(scratch1, offset(st, vec2(-1,-1),resolution)).r<0) flag=true;
	if (flag) fc +=512;
)"); //TODO less samples
		program = ShaderProgram::builder()
				.addShader(vertex2D->getCode(), GL_VERTEX_SHADER)
				.addShader(shader->getCode(), GL_FRAGMENT_SHADER)
				.link();
		program->bind();
		p->setCanvasUniforms(program);
		p->apply(program,p->get_scratch2());
		//p->get_scratch2()->swap(p->get_terrain());

		data = p->get_scratch2()->downloadDataRAW();
		width = p->getWidth();
		height = p->getHeight();
	});
	

//Find points of interest
std::cout << ((float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0)-(c1) << "\n";
std::cout << "Points of interest\n";
c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;
	std::mutex mtx;
	std::vector<std::pair<int,int>> ofInterest;

	std::function<void(std::pair<int,int>)> f = [this,&mtx,&ofInterest](std::pair<int,int> a) {
		std::vector<std::pair<int,int>> newOfInterest;
		int lower=a.first;
		for (int i=a.first; i<a.second; i++) {
			if(data[i]<0) { //not interesting
				if (i>lower) newOfInterest.emplace_back(lower,i);
				lower = i+1;
			}
		}
		//transfer to global
		mtx.lock();
		for (auto i : newOfInterest) ofInterest.emplace_back(i);
		mtx.unlock();
	};
	std::vector<std::pair<int,int>> jobs;
	for (int i=0; i<height; i++) jobs.emplace_back(i*width,(i+1)*width);
	cputools2::threadpool(f,jobs);

//Make list of lakes
std::cout << ((float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0)-(c1) << "\n";
std::cout << "Indexing lakes\n";
c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;

	auto lakes = std::make_unique<std::vector<std::vector<int>>>();

	f = [this,&mtx,lakes=lakes.get()](std::pair<int,int> a) {
		std::vector<int> new_lakes;
		for (int i=a.first; i<a.second; i++) {
			int d = (int) data[i];
			if (Nthbit(d,5)) new_lakes.emplace_back(i);

		}
		mtx.lock();
		lakes->emplace_back(new_lakes);
		mtx.unlock();
	};
	cputools2::threadpool(f,ofInterest);


std::cout << ((float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0)-(c1) << "\n";
std::cout << "Assigning lakes\n";
c1 = (float) (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000;

	auto lakeID = std::make_unique<float[]>(width*height);
	std::function<void(std::vector<int>)> f2 = [this, b=lakeID.get()](std::vector<int> a) {
		for (int lake : a) {
			std::stack<int> stack;
			stack.push(lake);
			while (!stack.empty()) {
				int s = stack.top();
				stack.pop();
				b[s] = lake*(pow(2,23)/width/height);
				std::cout << s%width << "," << s/width << " points to: ";
				for (auto n : neighbours(s,data[s])) std::cout << n%width << "," <<n/width << " ";
				std::cout << "\n";
				for (auto n : neighbours(s,data[s])) {
					stack.push(n);
					//std::cout << stack.size() << "\n";
					//std::cout << "push " << n%width << "," <<n/width << " from " << s%width << "," << s/width << " " << s << "\n";
				}
			}
		}
	};
	cputools2::threadpool(f2,*lakes);


	dispatchGPU([this,&lakeID](Project* p){
		auto a = p->get_scratch2()->downloadDataRAW();
		a[129000] = -a[129000];
		p->get_terrain()->uploadData(GL_RED,GL_FLOAT,a.get());


		//auto a = p->get_terrain()->downloadDataRAW();
		//for (int i=width*height; i>width; i--) a[i] = a[i-width];
		//p->get_terrain()->uploadData(GL_RED,GL_FLOAT,lakeID.get());
	});



	setProgress({true,1.0f});

}


bool FlowFilter::Nthbit(int num, int N) {
	return num & (1 << (N-1));
}


template<typename T>
void cputools2::threadpool(std::function<void(T)> f, std::vector<T> arg) {
	std::mutex mtx;
	uint Nthreads = std::thread::hardware_concurrency();
	Nthreads = 1;
	auto it = arg.begin();
	auto end = arg.end();

	auto j = [f,&arg,&mtx,&it,&end](){
		while (true) {
			mtx.lock();
			if (it==end) {
				mtx.unlock();
				return;
			}
			T a = *it;
			it++;
			mtx.unlock();
			f(a);
		}
	};
	std::vector<std::unique_ptr<std::thread>> threads;
	for (uint i=0; i<Nthreads; i++) threads.push_back(std::make_unique<std::thread>(j));
	for (auto &t : threads) t->join();
	for (auto &t : threads) t.reset();

}
