#ifndef RECIPEEDITOR_H
#define RECIPEEDITOR_H

#include <Window.h>
#include <View.h>
#include <Messenger.h>
#include <Button.h>
#include <Menu.h>
#include <String.h>
#include <TextView.h>
#include <TextControl.h>
#include <ListView.h>
#include <vector>
#include "AutoTextControl.h"
#include "RecipeFuncs.h"

#define M_EDIT_RECIPE 'edrc'
#define M_ADD_RECIPE 'adrc'
#define M_SET_RECIPE 'strc'
#define M_RECIPES_CHANGED 'rcch'

class RecipeEditor : public BWindow
{
public:
	RecipeEditor(const BRect &frame, const BMessenger &msgr,
				const int32 &number, const char *category);
	void MessageReceived(BMessage *msg);
	void FrameResized(float w, float h);

private:
	void AddIngredientRow(void);
	void RemoveIngredientRow(void);
	void PopulateIngredientList(void);
	void SaveAndClose(bool editing);

	BMessenger fMessenger;
	
	BMenu		*fCategories;
	
	AutoTextControl	*fNameBox;
	BTextControl	*fQuantityBox;
	BMenu		*fMeasurements,
				*fIngredients;
	BListView	*fIngredientList;
	BTextView	*fIngredientNotes,
				*fDirectionsBox;
	std::vector<IngredientRecord> fIngredientRecords;
	
	int32			fNumber;
	BString			fCategory;
	
	BButton		*fOK,
				*fCancel,
				*fAddIngredient,
				*fRemoveIngredient;
};

#endif
