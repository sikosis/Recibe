#include <stdlib.h>
#include <ScrollBar.h>
#include "SplitterView.h"
#include <stdio.h>

VSplitterView::VSplitterView(BRect frame, const char *name,uint32 resizingMode, uint32 flags)
 : BView(frame, name, resizingMode, flags),
	fLeftView(NULL),
	fRightView(NULL),
	fTracking(false),
	fSplitFactor(0.50),
	fSplitLineWidth(4.0)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	fLeftFrame = Bounds();
	fRightFrame = Bounds();

	fLeftFrame.right /= 2;
	fLeftFrame.right -= fSplitLineWidth/2;
	fSplitLineRect.Set(fLeftFrame.right + 1,0,fLeftFrame.right + fSplitLineWidth,Bounds().bottom);
	fRightFrame.left = fSplitLineRect.right + 1;
}	

VSplitterView::~VSplitterView(void)
{
}

void VSplitterView::AddChild(BView *aView, uint32 pane)
{
	if(pane != C_LEFT && pane != C_RIGHT)
		return;

	if ( (fLeftView && pane == C_LEFT) || (fRightView && pane == C_RIGHT) )
		debugger("You cannot add a view to a pane which already owns a view");

	if(pane == C_LEFT)
	{
		fLeftView = aView;
		aView->ResizeTo(fLeftFrame.right,fLeftFrame.bottom);
		fLeftView->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM );
	}
	else
	{
		fRightView = aView;
		aView->MoveTo(fRightFrame.left,0);
		fRightView->SetResizingMode(B_FOLLOW_ALL);
	}

	BView::AddChild(aView);
}

bool VSplitterView::RemoveChild(BView *aView)
{
	if(aView == fRightView)
		fRightView = NULL;
	else
	if(aView == fLeftView)
		fLeftView = NULL;
	else
		return false;
	BView::RemoveChild(aView);
	
	return true;
}

BRect VSplitterView::PaneBounds(uint8 which)
{
	if(which == C_RIGHT)
		return fRightFrame.OffsetToCopy(0,0);
	return fLeftFrame.OffsetToCopy(0,0);
}

BRect VSplitterView::PaneFrame(uint8 which)
{
	if(which == C_RIGHT)
		return fRightFrame;
	return fLeftFrame;
}

void VSplitterView::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	if(fTracking)
	{
		if(point.x < 1)
			point.x = 1;
		else
		if(point.x>Bounds().right - fSplitLineWidth)
			point.x = Bounds().right - fSplitLineWidth;
		
		float delta = fSplitLineRect.left - point.x;
		
		// recalculate split factor
		fSplitLineRect.OffsetTo(point.x,0);
		fSplitFactor = fSplitLineRect.left/Bounds().right;
		
		// recalculate bounds of children
		fLeftFrame.right = point.x - 1;
		fRightFrame.left = fSplitLineRect.right + 1;
		
		if(fRightView)
		{
			fRightView->MoveTo(fRightFrame.left,0);
//			fRightView->ResizeTo(fRightFrame.right,fRightFrame.bottom);

			// This *should* fix the resizing bug which caused us to lose our scrollbar
			// in the FileView when moving the divider
			fRightView->ResizeBy(delta,0);
		}
		if(fLeftView)
			fLeftView->ResizeTo(fLeftFrame.right,fLeftFrame.bottom);

		Draw(fSplitLineRect);
	}
}

void VSplitterView::MouseDown(BPoint point)
{
	if(fTracking)
		fTracking = false;
	
	if(fSplitLineRect.Contains(point))
	{
		fTracking = true;
//		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	}
}

void VSplitterView::MouseUp(BPoint point)
{
	if(fTracking)
		fTracking = false;
}

void VSplitterView::FrameResized(float width, float height)
{
	if(fLeftView)
		fLeftFrame = fLeftView->Bounds();
	if(fRightView)
		fRightFrame = fRightView->Frame();
}

void VSplitterView::Draw(BRect update)
{
	BRect r(Bounds());
	r.bottom = r.top = (r.bottom / 2);
	r.bottom += 25;
	r.top -= 25;
	r.left += 5;
	r.right -= 5;
	
	SetHighColor(tint_color(ViewColor(),B_DARKEN_4_TINT));
	FillRect(r,B_MIXED_COLORS);
	StrokeRect(r,B_SOLID_HIGH);
}

void VSplitterView::SetDividerWidth(float dwidth)
{
	// set the width of the divider line

	if(dwidth < 4 || dwidth>Bounds().Width()/2)
		return;
	
	fSplitLineRect.right = fSplitLineRect.left + dwidth;
	fRightFrame.left = fSplitLineRect.right + 1;
	
	if(fRightView)
	{
		fRightView->MoveTo(fRightFrame.left,0);
		fRightView->ResizeTo(fRightFrame.right,fRightFrame.bottom);
	}
	fSplitLineWidth = dwidth;

	Draw(fSplitLineRect);
}

void VSplitterView::SetSplitPoint(float spoint)
{
	// Takes a percentage value >0 and  < 1 which determines how much
	// room the left and right panes take up

	if(spoint <= 0 || spoint >= 1.0)
		return;
	
	// recalculate split factor
	fLeftFrame.right = Bounds().right * spoint;
	fLeftFrame.right -= fSplitLineWidth / 2;
	fSplitLineRect.Set(fLeftFrame.right + 1,0,fLeftFrame.right + 1 + fSplitLineWidth,Bounds().bottom);
	fRightFrame.left = fSplitLineRect.right + 1;

	fSplitFactor = spoint;

	if(fRightView)
	{
		fRightView->MoveTo(fRightFrame.left,0);
		fRightView->ResizeTo(fRightFrame.right,fRightFrame.bottom);
	}
	if(fLeftView)
		fLeftView->ResizeTo(fLeftFrame.right,fLeftFrame.bottom);

	Draw(fSplitLineRect);
}
