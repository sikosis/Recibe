#ifndef ADDCATWINDOW_H
#define ADDCATWINDOW_H

#include <Window.h>
#include <View.h>
#include <Messenger.h>
#include <Button.h>
#include "AutoTextControl.h"

#define M_ADD_CATEGORY 'adct'


class AddCatWindow : public BWindow
{
public:
	AddCatWindow(const BRect &frame, const BMessenger &msgr);
	void MessageReceived(BMessage *msg);

private:
	BMessenger fMessenger;
	
	AutoTextControl	*fNameBox;
	
	BButton		*fOK,
				*fCancel;
};

#endif
