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
		std::vector<int> pointed_to;
	};
	std::map<int,pointdata> index;
	std::set<std::pair<int,bool>> sinks;

//Find points of interest, and connect edges
	std::function<void(std::pair<int,int>)> f = [this,level,&lock,&index,coordof,indexof,&sinks](std::pair<int,int> a) {
		std::map<int,pointdata> new_index;
		std::set<std::pair<int,bool>> new_sinks;
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
				new_sinks.insert({i,data[min_i]<=level || selection[min_i]==0});
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

//Identify lakes, and pointed to
	int i=0;
	for (auto s : sinks) {
		index[s.first].sink = i;
		i++;
	}

	std::function<void(int)> f2 = [this,&index,coordof,indexof](int sink) {
		std::stack<int> stack;
		stack.push(sink);
		int sid = index[sink].sink;
		while (!stack.empty()) {
			int s = stack.top();
			stack.pop();
			//data[s] = sid;
			index[s].sink = sid;
			for (int dx : {-1,0,1}) for (int dy : {-1,0,1}) {
				auto coord = coordof(s);
				int id = indexof(coord.first + dx, coord.second + dy);
				if (index.count(id)==0 || id==s) continue;
				if (index[id].points_to == s) {
					index[s].pointed_to.push_back(id);
					stack.push(id);
				}
			}
		}
	};
	std::vector<int> sinkjobs;
	for (auto s : sinks) sinkjobs.emplace_back(s.first);
	cputools::threadpool(f2,sinkjobs);



//Lake connections
	struct connection {
		int fromLocation;
		int toLocation;
		int sinkID;
		float height;
	};
	std::map<int,std::map<int,connection>> connection_map;

	f2 = [this,&index,coordof,indexof,&lock,&connection_map](int sink) {
		std::map<int,connection> cons;
		std::stack<int> stack;
		stack.push(sink);
		int sid = index[sink].sink;
		while (!stack.empty()) {
			int s = stack.top();
			stack.pop();
			for (int dx : {-1,0,1}) for (int dy : {-1,0,1}) {
					auto coord = coordof(s);
					int id = indexof(coord.first + dx, coord.second + dy);
					if (index.count(id)==0 || id==s) continue;
					if (index[id].sink != sid) {
						if (cons.count(index[id].sink)==0) {
							cons[index[id].sink] = {s,id,index[id].sink,data[id]};
						} else if (data[id]<cons[index[id].sink].height) {
							cons[index[id].sink] = {s,id,index[id].sink,data[id]};
						}
					}
					if (index[id].points_to == s) {
						stack.push(id);
					}
				}
		}
		lock.lock();
		connection_map[sid] = cons;
		lock.unlock();
	};
	sinkjobs.erase(sinkjobs.begin(),sinkjobs.end());
	for (auto s : sinks) sinkjobs.emplace_back(s.first);
	cputools::threadpool(f2,sinkjobs);

//Add all candidates on border
	std::set<int> addedSinks;
	auto comp = [](const connection& c1, const connection& c2){return c1.height<c2.height;};
	std::set<connection,decltype(comp)> a(comp);
	for (auto s : sinks) {
		if (!s.second) continue;
		addedSinks.insert(index[s.first].sink);
		std::cout << "adding osink: " << index[s.first].sink << " " << sinks.count({s.first,true}) << "\n";
		for (auto o : connection_map[index[s.first].sink]) {
			a.insert(connection_map[o.first][index[s.first].sink]);
		}
	}

	while (!a.empty()) {
		connection c = *a.begin();
		a.erase(c);
		if (addedSinks.count(index[c.fromLocation].sink)>0) {
			//std::cout << index[c.fromLocation].sink << " already added\n";
			continue;
		}
		std::cout << "adding sink: " << index[c.fromLocation].sink << " connecting to " << index[c.toLocation].sink << " " << sinks.count({c.toLocation,true}) << "\n";
		//index[c.toLocation].sink = index[c.fromLocation].sink;

		index[c.toLocation].pointed_to.push_back(c.fromLocation);
		//index[c.fromLocation].pointed_to.push_back(c.toLocation);
		data[c.toLocation] = -1.0;

		addedSinks.insert(index[c.fromLocation].sink);
		for (auto o : connection_map[index[c.fromLocation].sink]) {
			//std::cout << "new possibility " << o.first << "\n";
			a.insert(connection_map[o.first][index[c.fromLocation].sink]);
		}
	}

//connect stuff
	f2 = [this,&index,coordof,indexof,&lock,&connection_map](int sink) {
		std::map<int,connection> cons;
		std::stack<int> stack;
		stack.push(sink);
		int sid = index[sink].sink;
		while (!stack.empty()) {
			int s = stack.top();
			stack.pop();
			for (auto n : index[s].pointed_to) stack.push(n);
			data[s] = sid*70238424%289;
		}
	};
	sinkjobs.erase(sinkjobs.begin(),sinkjobs.end());
	for (auto s : sinks) if(s.second) sinkjobs.emplace_back(s.first);
	cputools::threadpool(f2,sinkjobs);



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
