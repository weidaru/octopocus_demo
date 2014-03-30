#include "gesture_manager.h"
#include "gesture.h"

#include <fstream>
#include <assert.h>
#include <stdlib.h>

GestureManager::GestureManager() {

}

GestureManager::~GestureManager() {
	for(GestureMap::const_iterator it=gestures.cbegin(); it!= gestures.cend(); it++) {
		delete it->second;
	}
}

bool GestureManager::Load(const std::string &file_name) {
	//TODO: Add better error checking.
	std::ifstream file;
	file.open(file_name);
	if(!file.is_open()) {
		file.close();
		return false;
	}
	while(!file.eof()) {
		char *temp;
		std::string name;
		getline(file, name);
		if(name == "")
			continue;
		std::string size_str;
		if(file.eof())
			return false;
		getline(file, size_str);
		int s = (int)strtol(size_str.c_str(), &temp, 0);
		if(*temp != '\0')
			return false;
		std::string buf;
		if(file.eof())
			return false;
		getline(file, buf);
		Gesture *gesture = new Gesture;
		int p = 0;
		for(int i=0; i<s; i++) {
			int q = p;
			bool error = false;
			while(buf[q] != ' ' && buf[q] != '\t') {
				if(buf[q] == '\0') {
					error = true;
					break;
				}
				q++;
			}
			if(error) {
				delete gesture;
				return false;
			}
			float x = (float)strtod(buf.substr(p, q-p).c_str(), &temp);
			if(*temp != 0) {
				delete gesture;
				return false; 
			}
			p = ++q;
			while(buf[q] != ' ' && buf[q] != '\t') {
				if(buf[q] == '\0') {
					error = true;
					break;
				}
				q++;
			}
			if(error) {
				delete gesture;
				return false;
			}
			float y = (float)strtod(buf.substr(p, q-p).c_str(), &temp);
			if(*temp != 0) {
				delete gesture;
				return false;
			}
			p = ++q;
			gesture->PushBack(x, y);
		}
		assert(gesture->Size() == s);
		gestures[name] = gesture;
	}
	file.close();
	return true;
}

bool GestureManager::Save(const std::string &file_name) const {
	std::ofstream file;
	file.open(file_name);
	if(!file.is_open()) {
		file.close();
		return false;
	}
	for(GestureMap::const_iterator it=gestures.cbegin(); it!= gestures.cend(); it++) {
		file<<it->first<<"\n";
		const  Gesture *cur = it->second;
		file<<cur->Size()<<"\n";
		for(int i=0; i<cur->Size(); i++) {
			const Gesture::Point &p = cur->Get(i);
			file<<p.x<<" "<<p.y<<" ";
		}
		file<<"\n";
	}
	file.close();
	return true;
}

void GestureManager::Put(const std::string &name, Gesture *g) {
	gestures[name] = g;
}

Gesture* GestureManager::Get(const std::string& name) {
	return gestures[name];
}

void GestureManager::GetAll(std::vector<std::pair<std::string, Gesture *> > *result) const {
	for(GestureMap::const_iterator it=gestures.cbegin(); it!= gestures.cend(); it++) {
		result->push_back(std::make_pair(it->first, it->second));
	}
}

int GestureManager::Size() const {
	return gestures.size();
}