#ifndef DELCATWINDOW_H
#define DELCATWINDOW_H

#include <Window.h>
#include <View.h>
#include <Button.h>
#include <Menu.h>
#include <TextView.h>
#include "AutoTextControl.h"

class DelCatWindow : public BWindow
{
public:
	DelCatWindow(const BRect &frame, const char *category);
	void MessageReceived(BMessage *msg);

private:
	AutoTextControl	*fConfirmBox;
	
	BButton		*fOK,
				*fCancel;
	
	BTextView	*fWarningLabel;
	
	BMenu		*fCategories;
};

#endif
