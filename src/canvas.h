#ifndef CANVAS_H_
#define CANVAS_H_

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include <set>
#include <vector>
#include <string>

class Gesture;
class Canvas;

//Some event.
class CanvasEvent;
wxDECLARE_EVENT(CANVAS_EVENT, CanvasEvent);

class CanvasEvent: public wxCommandEvent
{
public:
	//self explain event id.
	enum {NEW_GESTURE=1, UPDATE_GESTURE, COMPLETE_GESTURE};

public:
	CanvasEvent(wxEventType commandType = CANVAS_EVENT, int id = 0);

	// You *must* copy here the data to be transported
	CanvasEvent(const CanvasEvent& event);

	// Required for sending with wxPostEvent()
	virtual wxEvent* Clone() const;

	CanvasEvent& SetCanvas(Canvas *new_canvas);
	Canvas *GetCanvas() const;
	
private:
	Canvas *canvas;
};

typedef void (wxEvtHandler::*CanvasEventFunction)(CanvasEvent &);
#define CanvasEventHandler(func) wxEVENT_HANDLER_CAST(CanvasEventFunction, func)                    
#define EVT_CANVAS(id, func) \
	wx__DECLARE_EVT1(CANVAS_EVENT, id, CanvasEventHandler(func))

/**
 *  See gesture for a brief design overview.
 *
 * Right now Canvas can only draw gesture and text.
 * If more things needs to be added in the future, we need to do some engineering.
 * Perhaps, in OOP manner, define some kind of interface called Drawable, then implement it.
 *
 * The Canvas also implement some event, @see CanvasEvent. The intention is to decouple 
 * the display of the new gesture and anything else, which could be drawing other stuffs and log 
 * the udpate of new gesture.
 */
class Canvas : public wxPanel {
private:
	struct Text {
		std::string data;
		float x, y;

		Text(const std::string _d, float _x, float _y) : data(_d), x(_x), y(_y) {

		}

		Text() {}
	};

private:
	typedef std::set<wxEvtHandler  *> Subscriptions;
	typedef std::vector<Gesture *> Gestures;
	typedef std::vector<Text> Texts;

public :
	Canvas(wxFrame *parent);

	void OnMouseLeftUp(wxMouseEvent &event);
	void OnMouseLeftDown(wxMouseEvent &event);

	void OnMouseMove(wxMouseEvent &event);

	Gesture *GetCurrentGesture() const { return cur_gesture; }
	void ClearCurrentGesture() { cur_gesture = 0; }

	/**
	  * Subscribe the event published by canvas for client.
	  */
	void Subscribe(wxEvtHandler *client);
	void Unsubscribe(wxEvtHandler *client);

	void ClearGesture();
	void DrawGeture(Gesture *g);

	void ClearText();
	void DrawText(std::string data, int x, int y);

private:
	enum MouseState {DOWN, UP};

private:
	void PaintEvent(wxPaintEvent &evt);
	void Publish(const wxEvent &event);
	DECLARE_EVENT_TABLE()

private:
	MouseState mouse_state;
	Gesture *cur_gesture;
	Subscriptions subscriptions;

	Gestures gestures;
	Texts texts;
};

#endif			//CANVAS_H_