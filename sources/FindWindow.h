#ifndef FINDWINDOW_H
#define FINDWINDOW_H

#include <Window.h>
#include <View.h>
#include <Messenger.h>
#include <Button.h>
#include <Menu.h>
#include "AutoTextControl.h"

#define M_FIND_RECIPE 'fnrc'


class FindWindow : public BWindow
{
public:
	FindWindow(const BRect &frame, const BMessenger &msgr,
			const char *name, const char *category, const char *ingred);
	void MessageReceived(BMessage *msg);

private:
	BMessenger fMessenger;
	
	AutoTextControl	*fNameBox;
	AutoTextControl	*fIngredientBox;
	
	BButton		*fOK,
				*fCancel;
	
	BMenu		*fCategories;
};

#endif
