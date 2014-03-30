#ifndef OCTOPOCUS_DEMO_H_
#define OCTOPOCUS_DEMO_H_

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "gesture_manager.h"

#include <vector>
#include <utility>

class MainFrame;
class CanvasEvent;
class Gesture;
class Canvas;

class OctopocusDemo: public wxApp
{
public:
	virtual bool OnInit();
};

class MainFrame: public wxFrame
{
private:
	typedef std::vector<std::pair<std::string, Gesture *> > Gestures;

public:
	MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
	GestureManager * GetManager() { return &manager; }

private:
	void OnOpen(wxCommandEvent& event);
	void OnExit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnNewGesture(CanvasEvent& event);
	void OnUpdateGesture(CanvasEvent& event);
	void OnCompleteGesture(CanvasEvent& event);


	void FeedForwardAndFeedBack(Gesture *cur, Canvas *canvas);
	float CalculateFalloff(Gesture *source, Gesture *target);

	DECLARE_EVENT_TABLE()

private:
	GestureManager manager;
	Gestures candiates;
};

#endif		//OCTOPOCUS_DEMO_H_