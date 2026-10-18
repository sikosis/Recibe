#ifndef _SPLITTERVIEW_H_
#define _SPLITTERVIEW_H_

#include <View.h>
#include <Rect.h>
#include <List.h>
#include <Cursor.h>

#define C_LEFT		0
#define C_RIGHT		1

class VSplitterView : public BView 
{
public:
		VSplitterView(BRect frame,const char *name = NULL,
			uint32 resize = B_FOLLOW_ALL,
			uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE);
		virtual ~VSplitterView(void);
		
		void AddChild(BView *aView, uint32 pane);
		bool RemoveChild(BView *aView);
		BRect PaneBounds(uint8 which);
		BRect PaneFrame(uint8 which);

		virtual void MouseMoved(BPoint point, uint32 transit, const BMessage *message);
		virtual void MouseDown(BPoint point);
		virtual void MouseUp(BPoint point);
		virtual void FrameResized(float width, float height);
		virtual void Draw(BRect update);

		float DividerWidth() const { return fSplitLineWidth; }
		void SetDividerWidth(float dwidth);
		float SplitPoint() const { return fSplitFactor; }
		void SetSplitPoint(float spoint);

private:
		BView	*fLeftView, *fRightView;
		bool	fTracking;
		float	fSplitFactor;
		float	fSplitLineWidth;
		BRect	fSplitLineRect,
				fLeftFrame,
				fRightFrame;
};

#endif