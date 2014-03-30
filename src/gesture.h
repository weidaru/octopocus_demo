#ifndef GESTURE_H_
#define GESTURE_H_

#include <vector>
#include <wx/pen.h>

class wxMemoryDC;

/**
 * It is better to abstract the Gesture to some extent.
 * But that should come together with our Canvas.
 * As Canvas cares about the generation of Gesture, it is aware of how Gesture is constructed.
 * In a word, they are tightly coupled and the decoupling will take a fair amount of code to achieve.
 * So I leave both the Gesture and Canvas as concrete class.
 * If this really needs to be capable of easy extension, play around on both Gesture and Canvas.
 *
 * For simplicity here, I am using a pixel vector to represent Gestures.
 * A Gesture is composed of a piecewise linear function and the compare function is measured by 
 * euclidean distance between two functions.
 */

class Gesture {
public:
	struct Point {
		float x, y;

		Point(float _x, float _y) : x(_x), y(_y) {}
		Point() : x(0.0f), y(0.0f) {}

		static inline float Distance(const Gesture::Point &lhs, const Gesture::Point &rhs) {
			return sqrt((float)((rhs.x-lhs.x)*(rhs.x-lhs.x) + (rhs.y-lhs.y)*(rhs.y-lhs.y)));
		}

		static inline float SquareDistance(const Gesture::Point &lhs, const Gesture::Point &rhs) {
			return (float)((rhs.x-lhs.x)*(rhs.x-lhs.x) + (rhs.y-lhs.y)*(rhs.y-lhs.y));
		}
	};

private:
	struct Meta {
		float p;
	};

	struct PenConfig {
		float p;
		wxPen pen;

		PenConfig() {}
		PenConfig(float _p, const wxPen &_pen) : p(_p), pen(_pen) {}
	};

private:
	typedef std::vector<Point> PointVector;		//Pixel is cheap to copy, no need to use pointer.
	typedef std::vector<Meta> MetaVector;		//Store some metadata for Point, for example parameterization of each point. 

public:
	Gesture();
	~Gesture();

	Gesture(const Gesture &rhs);
	void operator=(const Gesture &rhs);
	
	void Render(wxMemoryDC &dc) const;
	void ClearPens();
	void SetPen( float start, const wxPen& pen);
	

	/****************Accessors and Mutators.****************/
	void PushBack(float x, float y);
	void PopBack();
	const Point& Front() const;
	const Point& Back() const;
	const Point& Get(int index) const;
	int Size() const;

	Point GetAnchor() const  { return anchor; }

	Gesture& SetTransform(float x, float y) { transform.x = x; transform.y = y; return *this;}
	Point GetTransform() { return transform; }

	/****************Compare related.****************/
	/**
	 * Measure how similar is two gestures measured in [0.0f, INFINITY].
	 * It measures average euclidean distance.
	 * Any of the gesture could be incomplete. The function will take the min length and truncate the other gesture
	 * with that length. Then make comparison.
	 */
	float Compare(const Gesture& rhs) const;

	float Length() const;

	/**
	 * Sample the gesture, which is a piecewise linear function.
	 * @ Parameter p 
	 *				p is the function parameter according to arc length. It range from 0.0 to 1.0
	 *				example: if p == 0.5, it is the mid point of the function. 
	 * @ Return sampled pixels.
	 */
	Point Sample(float p) const;

	/**
	 * A batch version of Sample in uniform maner, which could be faster. @See Sample()
	 * It is possible to sample part of the function, start and end are used to control that.
	 * Note: the result will always contain sample_size number of elements.
	 */
	void UniformSample(int sample_size, std::vector<Gesture::Point> *result, float start = 0.0f, float end = 1.0f) const;

private:
	//Parameterization according to arc length. 
	void Parameterization() const;
	static int BinarySearch(const std::vector<Gesture::Meta> &input, const Gesture::Meta &val);
	Point Sample(int left, int right, float p) const;

private:
	PointVector points;
	Point anchor;
	Point transform;

	//Render related.
	std::vector<PenConfig> pens;

	 //Do lazy evaluation.
	mutable bool need_reparam;
	mutable float length;
	mutable MetaVector metas;
};

#endif			//GESTURE_H_