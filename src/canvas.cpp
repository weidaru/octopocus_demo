#include "canvas.h"
#include "gesture.h"

#include "gesture_manager.h"

#include <wx/dcbuffer.h>

Canvas::Canvas(wxFrame* parent) : 
			wxPanel(parent), mouse_state(UP), cur_gesture(0) {
	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void Canvas::PaintEvent(wxPaintEvent & evt) {
	wxAutoBufferedPaintDC dc(this);
	
	//Clear
	dc.SetBackground(*wxMEDIUM_GREY_BRUSH);
	dc.Clear();
	
	//Render gestures.
	for(int i=0; i<gestures.size(); i++) {
		Gesture *cur = gestures[i];
		Gesture::Point trans = cur->GetTransform();
		cur->SetTransform(trans.x, trans.y);
		cur->Render(dc);
	}

	//Render texts.
	for(int i=0; i<texts.size(); i++) {
		dc.DrawText(texts[i].data.c_str(), texts[i].x, texts[i].y);
	}

	//Render current gesture.
	if(cur_gesture) {
		Gesture::Point anchor = cur_gesture->GetAnchor();
		cur_gesture->SetTransform(anchor.x, anchor.y);
		cur_gesture->Render(dc);
	}
}

void Canvas::OnMouseLeftDown(wxMouseEvent& event) {
	mouse_state = DOWN;
	if(cur_gesture)
		delete cur_gesture;
	cur_gesture = new Gesture;
	cur_gesture->PushBack(event.GetX(), event.GetY());

	CanvasEvent e(CANVAS_EVENT, CanvasEvent::NEW_GESTURE);
	e.SetCanvas(this);
	Publish(e);
	Refresh();
}

void Canvas::OnMouseLeftUp(wxMouseEvent &event) {
	mouse_state = UP;

	CanvasEvent e(CANVAS_EVENT, CanvasEvent::COMPLETE_GESTURE);
	e.SetCanvas(this);
	Publish(e);

	Refresh();
}

void Canvas::OnMouseMove(wxMouseEvent& event) {

	//Stub
	switch(mouse_state) {
	case DOWN:
		{
			int x = event.GetX(), y = event.GetY();
			cur_gesture->PushBack(x, y);
			
			CanvasEvent e(CANVAS_EVENT, CanvasEvent::UPDATE_GESTURE);
			e.SetCanvas(this);
			Publish(e);

			Refresh();
		}
		break;
	default:
		break;
	}
}

void Canvas::Subscribe(wxEvtHandler *client) {
	subscriptions.insert(client);
}

void Canvas::Unsubscribe(wxEvtHandler *client) {
	subscriptions.erase(client);
}

void Canvas::Publish(const wxEvent &event) {
	for(Subscriptions::iterator it=subscriptions.begin(); it!=subscriptions.end(); it++) {
		wxPostEvent(*it, event);
	}
}

void Canvas::ClearGesture() {
	gestures.clear();
}

void Canvas::DrawGeture(Gesture *g) {
	gestures.push_back(g);
}

void Canvas::ClearText() {
	texts.clear();
}

void Canvas::DrawText(std::string data, int x, int y) {
	texts.push_back(Text(data, x, y));
}

//Event define
wxDEFINE_EVENT(CANVAS_EVENT, CanvasEvent);

BEGIN_EVENT_TABLE(Canvas, wxPanel)
	EVT_PAINT(Canvas::PaintEvent)
	EVT_LEFT_DOWN(Canvas::OnMouseLeftDown)
	EVT_LEFT_UP(Canvas::OnMouseLeftUp)
	EVT_MOTION(Canvas::OnMouseMove)
END_EVENT_TABLE()

CanvasEvent::CanvasEvent(wxEventType type, int id) : wxCommandEvent(type, id) {

}

// You *must* copy here the data to be transported
CanvasEvent::CanvasEvent(const CanvasEvent& event) : wxCommandEvent(event) {
	this->canvas = event.GetCanvas();
}

// Required for sending with wxPostEvent()
wxEvent* CanvasEvent::Clone() const {
	return new CanvasEvent(*this);
}

CanvasEvent& CanvasEvent::SetCanvas(Canvas *new_canvas) {
	canvas = new_canvas;
	return *this;
}

Canvas *CanvasEvent::GetCanvas() const {
	return canvas;
}
