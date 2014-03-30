#include "gesture.h"

#include <math.h>

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include <wx/graphics.h>

Gesture::Gesture() : need_reparam(false) {
	pens.push_back(PenConfig(0.0f, wxPen(wxColor(0, 0, 0), 1)));
}
Gesture::~Gesture() {

}

Gesture::Gesture(const Gesture &rhs) {
	this->operator=(rhs);
}

void Gesture::operator=(const Gesture &rhs) {
	this->points = rhs.points;
	this->need_reparam = true;
}

const static int kMaxSampleSize = 100;
const static float kErrorClamp = 5.0f;

float Gesture::Compare(const Gesture& rhs) const {
	if(rhs.Size() <= 1 || Size() <= 1)
		return 0.0f;

	float left_l = Length(), right_l = rhs.Length();
	const Gesture *left = this, *right = &rhs;
	
	//Make sure left_l < right_l;
	if(left_l > right_l) {
		const Gesture *temp = left;
		left = right;
		right = temp;
		float dummy = left_l;
		left_l = right_l;
		right_l = dummy;
	}

	std::vector<Point> left_sample, right_sample;
	int sample_size = left_l/right_l*kMaxSampleSize;
	sample_size = sample_size < 2 ? 2 : sample_size;
	left->UniformSample(sample_size, &left_sample, 0.0f, 1.0f);
	right->UniformSample(sample_size, &right_sample, 0.0f, left_l/right_l);
	
	float error = 0.0f;
	for(int i=0; i<sample_size; i++) {
		float  d = Point::SquareDistance(left_sample[i], right_sample[i]);
		if(d > kErrorClamp)
			error += d;
	}

	return sqrt(error/sample_size);
}

void Gesture::Render(wxMemoryDC &dc) const {
	if(points.empty())
		return;
	Parameterization();

	wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
	assert(gc);

	//Gather index for each pen.
	std::vector<int> start_indices;
	//Assume the interval of piecewise function is small.
	int pen_index = 1;
	start_indices.push_back(0);
	for(int i=0; i<points.size()-1; i++) {
		float p = metas[i+1].p;
		if(pen_index >= pens.size())
			break;
		if(p > pens[pen_index].p) {
			start_indices.push_back(i);
			pen_index++;
		}
	}

	assert(start_indices.size() == pens.size());

	for(int i=0; i<start_indices.size() - 1; i++) {
		if(pens[i].pen.GetWidth() == 0)
			continue;

		int p_start = start_indices[i];
		int p_end = start_indices[i+1];
		gc->SetPen(pens[i].pen);
		wxGraphicsPath path = gc->CreatePath();
		for(int j=p_start; j<p_end; j++) {
			path.MoveToPoint(points[j].x+transform.x, points[j].y+transform.y);
			path.AddLineToPoint(points[j+1].x+transform.x, points[j+1].y+transform.y);
		}
		gc->StrokePath(path);
	}


	if(pens.back().pen.GetWidth() != 0) {
		gc->SetPen(pens.back().pen);
		wxGraphicsPath path = gc->CreatePath();
		for(int i=start_indices.back(); i<points.size()-1; i++) {
			path.MoveToPoint(points[i].x+transform.x, points[i].y+transform.y);
			path.AddLineToPoint(points[i+1].x+transform.x, points[i+1].y+transform.y);
		}
		gc->StrokePath(path);
	}

	delete gc;
}

void Gesture::ClearPens() {
	pens.erase(pens.begin()+1, pens.end());
}

void Gesture::SetPen(float start, const wxPen& pen) {
	//Just do linear search here.
	if(start == 0.0f) {
		pens[0].pen = pen;
		return;
	}

	assert(start>0.0f && start<1.0f);
	for(int i=0; i<pens.size(); i++) {
		if(pens[i].p  > start) {
			pens.insert(pens.begin()+i, PenConfig(start, pen));
			return;
		}
	}
	pens.push_back(PenConfig(start, pen));
}


void Gesture::PushBack(float x, float y) {
	points.push_back(Point(x, y));
	need_reparam = true;

	if(points.size() == 1) {
		anchor.x = x;
		anchor.y = y;
	}

	//Transform according to the first element.
	points.back().x -= anchor.x;
	points.back().y -= anchor.y;
	if(points.size() > 1) {
		int n = points.size();
		if(points[n-1].x == points[n-2].x && points[n-1].y==points[n-2].y)
			PopBack();
	}
}

void Gesture::PopBack() {
	points.pop_back();
	need_reparam = true;
}

const Gesture::Point& Gesture::Front() const {
	return points.front();
}

const Gesture::Point& Gesture::Back() const {
	return points.back();
}

const Gesture::Point& Gesture::Get(int index) const {
	return points[index];
}

int Gesture::Size() const {
	return (int)points.size();
}

float Gesture::Length() const {
	Parameterization();
	return length;
}

 /**
	* Simple binary search for Meta according to p. Return the index of largest element that are no greater than val.
	* Any p out of (0.0, 1.0) will trigger assertion.
	*/
int Gesture::BinarySearch(const std::vector<Gesture::Meta> &input, const Gesture::Meta &val) {
	int left=0, right=input.size()-1;

	assert(val.p > 0.0f && val.p < 1.0f);

	while(right-left > 1) {
		int mid = (left+right)/2;
		if(input[mid].p == val.p)
			return mid;
		else if(input[mid].p <val.p) {
			left = mid;
		}
		else {
			right = mid;
		}
	}

	return left;
}

namespace {
	Gesture::Point Lerp(const Gesture::Point &lhs, const Gesture::Point &rhs, float p) {
		Gesture::Point result;
		result.x = lhs.x * (1-p) + rhs.x * p;
		result.y = lhs.y * (1-p) + rhs.y * p;
		return result;
	}
}

Gesture::Point Gesture::Sample(float p) const {
	assert(p>=0.0f && p <= 1.0f);
	Parameterization();
	if(p == 0.0f)
		return points.front();
	if(p == 1.0f)
		return points.back();

	Meta dummy;
	dummy.p = p;
	int left_index = BinarySearch(metas, dummy);

	return Sample(left_index, left_index+1, p);
}

Gesture::Point Gesture::Sample(int left, int right, float p) const {
	assert(right > left);
	float left_p = metas[left].p, right_p = metas[right].p;
	
	assert(p>=left_p && p<=right_p);
	float new_p =  (p-left_p)/(right_p - left_p);

	//Do linear interpolation.
	return Lerp(points[left], points[right], new_p);
}

void Gesture::UniformSample(int sample_size, std::vector<Gesture::Point> *result, float start, float end) const {
	assert(sample_size >= 2);
	assert(end > start && start >=0.0f && end <= 1.0f);
	assert(points.size() > 1);
	Parameterization();

	int left_index = 0, right_index = points.size()-2;
	if(start != 0.0f) {
		Meta dummy;
		dummy.p = start;
		left_index = BinarySearch(metas, dummy);
	}
	if(end != 1.0f) {
		Meta dummy;
		dummy.p = end;
		right_index = BinarySearch(metas, dummy);
	}

	float interval = (end - start)/(float)sample_size;
	result->clear();
	result->push_back(Sample(left_index, left_index+1, start));
	int cur = left_index;
	for(int i=1; i<sample_size-1; i++) {
		float p = start + interval*i;
		while(metas[cur+1].p < p ) {
			cur++;
			assert(cur <= right_index);
		}
		result->push_back(Sample(cur, cur+1, p));
	}
	result->push_back(Sample(right_index, right_index+1, end));
	assert(result->size() == sample_size);
}

void Gesture::Parameterization() const {
	if(!need_reparam)
		return;
	if(points.empty()) {
		length = 0.0f;
		metas.clear();
		need_reparam = false;
		return;
	}

	metas.resize(points.size());
	length = 0.0f;
	metas[0].p = length;
	for(int i=1; i<points.size(); i++) {
		length += Point::Distance(points[i-1], points[i]);
		metas[i].p = length;
	}
	//normalize 
	if(length != 0.0f) {
		for(int i=0; i<points.size(); i++) {
			metas[i].p /= length;
		}
	}
	need_reparam = false;
}