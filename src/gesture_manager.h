#ifndef GESTURE_MANAGER_H_
#define GESTURE_MANAGER_H_

#include <map>
#include <string>
#include <vector>
#include <utility>

class Gesture;

//A simple manager to do serialization and deserialization.
class GestureManager {
private:
	typedef std::map<std::string, Gesture *> GestureMap;

public:
	GestureManager();
	~GestureManager();

	bool Load(const std::string &file_name);
	bool Save(const std::string &file_name) const;

	//Will delete g when destroyed.
	void Put(const std::string &name, Gesture *g);
	Gesture* Get(const std::string &name);
	void GetAll(std::vector<std::pair<std::string, Gesture *> > *result) const;
	int Size() const;

private:
	GestureMap gestures;	
};

#endif				//GESTURE_MANAGER_H_