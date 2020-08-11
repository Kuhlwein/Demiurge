//
// Created by kuhlwein on 8/10/20.
//

#include "Project.h"
#include <imgui/imgui.h>
#include <iostream>
#include <thread>
#include "cpufilter.h"

cpufilterMenu::cpufilterMenu() : FilterModal("cpufilter") {

}

void cpufilterMenu::update_self(Project *p) {
	ImGuiIO io = ImGui::GetIO();

	const char* items[] = { "Erode","Dilate"};
	static int current = 0;
	ImGui::Combo("Operation",&current, items,IM_ARRAYSIZE(items));
}

std::shared_ptr<BackupFilter> cpufilterMenu::makeFilter(Project *p) {
	auto morph = new cpufilter(p);
	return std::make_shared<ProgressFilter>(p,[](Project* p){return p->get_terrain();},morph);
}



cpufilter::cpufilter(Project *p) {

}

void cpufilter::setup(Project *p) {
	data = p->get_terrain()->downloadDataRAW();
	selection = p->get_selection()->downloadDataRAW();
	width = p->getWidth();
	height = p->getHeight();
}

void cpufilter::run() {

	auto indexof = [this](int x, int y){return y*width+x;};
	auto coordof = [this](int i){return std::pair<int,int>{i%width,i/width};};


	float level = 0.0f;


	std::mutex lock;
	struct pointdata {
		int points_to;
		int lake;
		int sink;
	};
	std::map<int,pointdata> index;
	std::set<int> sinks;

//Find points of interest, and connect edges
	std::function<void(std::pair<int,int>)> f = [this,level,&lock,&index,coordof,indexof,&sinks](std::pair<int,int> a) {
		std::map<int,pointdata> new_index;
		std::set<int> new_sinks;
		for (int i=a.first; i<a.second; i++) {
			if(data[i]<=level || selection[i]==0) continue;

			float minval = MAXFLOAT;
			int min_i;
			for (int dx : {-1,0,1}) for (int dy : {-1,0,1}) {
				auto coord = coordof(i);
				int id = indexof(coord.first+dx,coord.second+dy);
				float val = data[id];
				if (val<minval) {
					minval = val;
					min_i = id;
				}
			}
			new_index[i] = {min_i,0};
			if (min_i == i || data[min_i]<=level || selection[min_i]==0) { //If self or border pixel, make sink
				new_sinks.insert(i);
			}
		}
		lock.lock();
		index.insert(new_index.begin(), new_index.end());
		sinks.insert(new_sinks.begin(),new_sinks.end());
		lock.unlock();
	};
	std::vector<std::pair<int,int>> jobs;
	for (int i=0; i<height; i++) jobs.emplace_back(i*width,(i+1)*width);
	cputools::threadpool(f,jobs);
	selection.reset();

	std::cout << index.size() << "\n";
	std::cout << sinks.size() << "\n";

//Identify lakes
	int i=0;
	for (auto s : sinks) {
		index[s].sink = i;
		i++;
	}

	std::function<void(int)> f2 = [this,&index,coordof,indexof](int sink) {
		std::stack<int> stack;
		stack.push(sink);
		int sid = index[sink].sink;
		while (!stack.empty()) {
			int s = stack.top();
			stack.pop();
			data[s] = sid;
			index[s].sink = sid;
			for (int dx : {-1,0,1}) for (int dy : {-1,0,1}) {
				auto coord = coordof(s);
				int id = indexof(coord.first + dx, coord.second + dy);
				if (index.count(id)==0 || id==s) continue;
				if (index[id].points_to == s) {
					stack.push(id);
				}
			}
		}
	};
	std::vector<int> sinkjobs;
	sinkjobs.insert(sinkjobs.begin(),sinks.begin(),sinks.end());
	cputools::threadpool(f2,sinkjobs);



//Lake connections
	struct connection {
		int toLocation;
		float height;
	};

//	//Make connections
//	f = [this,&index,coordof,indexof](std::pair<int,int> a) {
//		for (int i=a.first; i<a.second; i++) {
//			if (index.count(i)==0) continue;
//			float minval = MAXFLOAT;
//			int min_i;
//			for (int dx : {-1,0,1}) for (int dy : {-1,0,1}) {
//				auto coord = coordof(i);
//				int id = indexof(coord.first+dx,coord.second+dy);
//				float val = data[id];
//				if (val<minval) {
//					minval = val;
//					min_i = id;
//				}
//			}
//			index[i].points_to = min_i;
//		}
//	};
//	for (int i=0; i<height; i++) jobs.emplace_back(i*width,(i+1)*width);
//	cputools::threadpool(f,jobs);


//	f = [this,&index,coordof,indexof](std::pair<int,int> a) {
//		for (int i=a.first; i<a.second; i++) {
//			if (index.count(i)==0) continue;
//			data[i]	= (int)index[i].points_to;
//		}
//	};
//	for (int i=0; i<height; i++) jobs.emplace_back(i*width,(i+1)*width);
//	cputools::threadpool(f,jobs);





	setProgress({true,1.0f});
}

void cpufilter::finalize(Project *p) {
	p->get_terrain()->uploadData(GL_RED,GL_FLOAT,data.get());

}


template<typename T>
void cputools::threadpool(std::function<void(T)> f, std::vector<T> arg) {
	std::mutex mtx;
	uint Nthreads = std::thread::hardware_concurrency();

	auto j = [f,&arg,&mtx](){
		while (true) {
			mtx.lock();
			if (arg.empty()) {
				mtx.unlock();
				return;
			}
			T a = arg.back();
			arg.pop_back();
			mtx.unlock();
			f(a);
		}
	};
	std::vector<std::unique_ptr<std::thread>> threads;
	for (uint i=0; i<Nthreads; i++) threads.push_back(std::make_unique<std::thread>(j));
	for (auto &t : threads) t->join();

}
