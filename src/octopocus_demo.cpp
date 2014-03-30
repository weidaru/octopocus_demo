#include "octopocus_demo.h"
#include "canvas.h"
#include "gesture.h"

#include <string>

#include <wx/dcbuffer.h>
#include <wx/filedlg.h>
#include <wx/wfstream.h>

bool OctopocusDemo::OnInit()
{
	MainFrame *main_frame = new MainFrame( "Octopocus Demo", wxPoint(200, 150), wxSize(800, 600) );

	wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);

	Canvas *canvas = new Canvas(main_frame);
	canvas->Subscribe(main_frame);
	sizer->Add(canvas, 1, wxEXPAND);

	main_frame->SetSizer(sizer);
	main_frame->SetAutoLayout(true);
	main_frame->Show( true );

	return true;
}

namespace {
	//Event ID.
	enum { myID_OPEN };
}

MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
	: wxFrame(NULL, wxID_ANY, title, pos, size)
{
	wxMenu *menuFile = new wxMenu;
	menuFile->Append(myID_OPEN, "&Open...\tCtrl-O",
		"Load gestures from file system.");
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);
	wxMenu *menuHelp = new wxMenu;
	menuHelp->Append(wxID_ABOUT);
	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append( menuFile, "&File" );
	menuBar->Append( menuHelp, "&Help" );
	SetMenuBar( menuBar );
	CreateStatusBar();
}



wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
	EVT_MENU(myID_OPEN, MainFrame::OnOpen)
	EVT_MENU(wxID_EXIT,  MainFrame::OnExit)
	EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
	EVT_CANVAS(CanvasEvent::NEW_GESTURE, MainFrame::OnNewGesture)
	EVT_CANVAS(CanvasEvent::UPDATE_GESTURE, MainFrame::OnUpdateGesture)
	EVT_CANVAS(CanvasEvent::COMPLETE_GESTURE, MainFrame::OnCompleteGesture)
wxEND_EVENT_TABLE()

void MainFrame::OnExit(wxCommandEvent& event)
{
	Close( true );
}

void MainFrame::OnAbout(wxCommandEvent& event)
{
	wxMessageBox( "This is a demo of mimicing Octopocus ",
		"About Octopocus", wxOK | wxICON_INFORMATION );
}

//Things that are configurable.
static const float kFeedForwardLength = 80.0f;
static const int kInitialWidth = 10;
static const unsigned char kTransparency =  20;
static const float kCancelThreshold = 20.0f;
static const float kErrorThreshod = 50.0f;
static const float kLengthUpThreshold = 1.2f;
static const float kLengthLowThreshold = 0.9f;

void MainFrame::FeedForwardAndFeedBack(Gesture *c, Canvas *canvas) {
	//TODO: Generate colors on the fly.
	std::vector<wxColor> colors;
	colors.push_back(*wxBLUE);
	colors.push_back(*wxCYAN);
	colors.push_back(*wxGREEN);
	colors.push_back(*wxYELLOW);
	colors.push_back(*wxRED);
	assert(candiates.size()<=5);		//Hard code colors of size 5.


	//Setup gestures to be displayed.
	Gesture::Point anchor = c->GetAnchor();
	canvas->ClearText();
	for(int i=0; i<candiates.size(); i++) {
		Gesture *cur = candiates[i].second;
		float falloff = CalculateFalloff(c, cur);

		cur->ClearPens();
		cur->SetPen(0.0f, wxPen(colors[i], 0));

		float ff_start = c->Length()/cur->Length();
		ff_start = ff_start > 1.0f ? 1.0f : ff_start;
		if(ff_start != 1.0f) {
			cur->SetPen(ff_start, wxPen(colors[i], kInitialWidth * falloff));
		}
		
		float ff_end = ff_start + kFeedForwardLength/cur->Length();
		ff_end = ff_end > 1.0f ? 1.0f : ff_end;

		
		if(ff_end != 1.0f) {
			wxColor trans(colors[i].Red(), colors[i].Green(), colors[i].Blue(), kTransparency);
			wxPen trans_pen(trans, kInitialWidth * falloff);
			cur->SetPen(ff_end, trans_pen);
		}

		//Set transform
		Gesture::Point transform =  c->Back();
		float p = c->Length()/cur->Length();
		p = p >1.0f ? 1.0f : p;
		Gesture::Point temp = cur->Sample(p);
		transform.x = -temp.x + transform.x + anchor.x;
		transform.y = -temp.y + transform.y + anchor.y;
		cur->SetTransform(transform.x, transform.y);

		//Try to draw candiates names.
		if(falloff == 0.0f)
			continue;
		else {
			Gesture::Point p = cur->Sample(ff_end);
			canvas->DrawText(candiates[i].first, p.x + transform.x, p.y + transform.y);
		}
	}
}

float MainFrame::CalculateFalloff(Gesture *source, Gesture *target) {
	if(source->Length() > target->Length()*kLengthUpThreshold)
		return 0.0f;
	float error = source->Compare(*target);
	if(error > kErrorThreshod)
		return 0.0f;
	else
		return 1.0f - error/kErrorThreshod;
}

void MainFrame::OnOpen(wxCommandEvent& event) {
	wxFileDialog 
		dialog(this, _("Open gesture file"), "", "",
		"gesture files (*.dat)|*.dat", wxFD_OPEN|wxFD_FILE_MUST_EXIST);
	if (dialog.ShowModal() == wxID_CANCEL)
		return;     // the user changed idea...

	// proceed loading the file chosen by the user;
	// this can be done with e.g. wxWidgets input streams:
	wxFileInputStream input_stream(dialog.GetPath());
	if (!input_stream.IsOk())
	{
		wxLogError("Cannot open file '%s'.", dialog.GetPath());
		return;
	}
	if(manager.Load(dialog.GetPath().ToStdString())) {
		char buf[128];
		sprintf(buf, "%d gestures loaded.", manager.Size());
		SetStatusText(buf);
	}
	else {
		SetStatusText("Gesture file is invalid.");
	}
}

void MainFrame::OnNewGesture(CanvasEvent& event) {
	Canvas *canvas = event.GetCanvas();
	Gesture *cur_gesture = canvas->GetCurrentGesture();
	if(!cur_gesture)		//As event are asynchronous, logic procedure sequence is not reliable. So check here.
		return;
	
	std::vector<std::pair<std::string, Gesture *> > entries;
	candiates.clear();
	manager.GetAll(&candiates);

	canvas->ClearGesture();
	for(int i=0; i<candiates.size(); i++) {
		canvas->DrawGeture(candiates[i].second);
	}
	
	FeedForwardAndFeedBack(cur_gesture, canvas);
	canvas->Refresh();
}

void MainFrame::OnUpdateGesture(CanvasEvent& event) {
	Canvas *canvas = event.GetCanvas();
	Gesture *cur_gesture = canvas->GetCurrentGesture();
	if(!cur_gesture)		//As event are asynchronous, logic procedure sequence is not reliable. So check here.
		return;
	FeedForwardAndFeedBack(cur_gesture, canvas);
}

void MainFrame::OnCompleteGesture(CanvasEvent& event) {
	Canvas *canvas = event.GetCanvas();
	Gesture *cur_gesture = canvas->GetCurrentGesture();
	if(!cur_gesture)			//As event are asynchronous, logic procedure sequence is not reliable. So check here.
		return;
	canvas->ClearCurrentGesture();
	canvas->ClearGesture();
	canvas->ClearText();

	//Check whether gesture is valid or get cancelled.
	Gesture::Point head=cur_gesture->Front();
	Gesture::Point tail = cur_gesture->Back();
	if(Gesture::Point::Distance(head, tail) < kCancelThreshold) {
		SetStatusText("Gesture Cancelled.");
	}
	else {
		//Check match
		int best_match=-1;
		float best_falloff = 0.0f;
		for(int i=0; i<candiates.size(); i++) {
			float temp = CalculateFalloff(cur_gesture, candiates[i].second);
			float length_ratio = cur_gesture->Length() / candiates[i].second->Length();

			if(length_ratio < kLengthLowThreshold || temp == 0.0f) {
				continue;
			}

			if(temp > best_falloff) {
				best_falloff = temp;
				best_match = i;
			}
		}

		if(best_match == -1) {
			SetStatusText("Gesture does not match");
		}
		else {
			char buf[128];
			sprintf(buf, "Match %s", candiates[best_match].first.c_str());
			SetStatusText(buf);
		}
	}
}