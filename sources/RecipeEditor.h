#ifndef RECIPEEDITOR_H
#define RECIPEEDITOR_H

#include <Window.h>
#include <View.h>
#include <Messenger.h>
#include <Button.h>
#include <Menu.h>
#include <String.h>
#include <TextView.h>
#include "AutoTextControl.h"

#define M_EDIT_RECIPE 'edrc'
#define M_ADD_RECIPE 'adrc'
#define M_SET_RECIPE 'strc'

class RecipeEditor : public BWindow
{
public:
	RecipeEditor(const BRect &frame, const BMessenger &msgr,
				const int32 &number, const char *category);
	void MessageReceived(BMessage *msg);
	void FrameResized(float w, float h);

private:
	BMessenger fMessenger;
	
	BMenu		*fCategories;
	
	AutoTextControl	*fNameBox;
	BTextView		*fIngredientBox,
					*fDirectionsBox;
	
	int32			fNumber;
	BString			fCategory;
	
	BButton		*fOK,
				*fCancel;
};

#endif
