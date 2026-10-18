#include "FindWindow.h"
#include "RecipeFuncs.h"
#include <Application.h>
#include <Font.h>
#include <MenuField.h>
#include <MenuItem.h>

enum
{
	M_NAME_CHANGED,
	M_INGREDIENTS_CHANGED,
	M_CATEGORY_CHANGED,
	M_QUIT_APP
};

FindWindow::FindWindow(const BRect &frame, const BMessenger &msgr,
						const char *name, const char *category, const char *ingred)
 :	BWindow(frame,"Find",B_FLOATING_WINDOW_LOOK, B_MODAL_SUBSET_WINDOW_FEEL,
 			B_ASYNCHRONOUS_CONTROLS),
 	fMessenger(msgr)
{
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	AddShortcut('Q', B_COMMAND_KEY, new BMessage(M_QUIT_APP));
	
	BView *back = new BView(Bounds(),"back",B_FOLLOW_ALL, B_WILL_DRAW);
	back->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(back);
	
	fCategories = new BMenu("Categories");
	CppSQLite3Query query = DBQuery("select category from categories order by category;",
									"FindWindow:get categories");
	while(!query.eof())
	{
		BString cat(DeescapeIllegalCharacters(query.getStringField(0)));
		fCategories->AddItem(new BMenuItem(cat.String(),new BMessage('dmmy')));
		query.nextRow();
	}
	fCategories->SetRadioMode(true);
	fCategories->SetLabelFromMarked(true);
	if(fCategories->CountItems()>0)
		fCategories->ItemAt(0)->SetMarked(true);
	BRect r(10,10,10 + fCategories->MaxContentWidth(),11);
	BMenuField *field = new BMenuField(r,"field","Category",fCategories);
	back->AddChild(field);
	field->ResizeToPreferred();
	r = field->Frame();
	
	BMenuItem *marked = fCategories->FindItem(category);
	if(marked)
		marked->SetMarked(true);
	
	r.OffsetBy(0,r.Height() + 10);
	float namewidth = be_plain_font->StringWidth("Name: ") + 5;
	fNameBox = new AutoTextControl(r,"namebox","Name ",name,
									new BMessage(M_NAME_CHANGED),
									B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fNameBox->SetEscapeCancel(true);
	back->AddChild(fNameBox);
	fNameBox->ResizeToPreferred();
	r = fNameBox->Frame();
	r.right = Bounds().right - 10;
	fNameBox->ResizeTo(r.Width(), r.Height());
	
	r.OffsetBy(0,r.Height() + 10);
	float ingredientwidth = be_plain_font->StringWidth("Ingredients: ") + 5;
	fIngredientBox = new AutoTextControl(r,"catbox","Ingredients ",ingred,
									new BMessage(M_INGREDIENTS_CHANGED),
									B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	back->AddChild(fIngredientBox);
	
	float divider = MAX(namewidth, ingredientwidth);
	fNameBox->SetDivider(divider);
	fIngredientBox->SetDivider(divider);
	
	fOK = new BButton(BRect(10,10,11,11),"ok","Cancel", new BMessage(M_FIND_RECIPE),
					B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fOK->ResizeToPreferred();
	fOK->SetLabel("OK");
	fOK->MoveTo(Bounds().right - fOK->Bounds().Width() - 10,
				Bounds().bottom - fOK->Bounds().Height() - 10);
	r = fOK->Frame();
	back->AddChild(fOK);
	fOK->MakeDefault(true);
	
	r.OffsetBy(-r.Width() - 10, 0);
	fCancel = new BButton(r,"cancel","Cancel",new BMessage(B_QUIT_REQUESTED),
						B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	back->AddChild(fCancel);
	
	field->MakeFocus(true);
	
	ResizeTo(Bounds().Width(),
			fIngredientBox->Frame().bottom + 20 + fOK->Bounds().Height());
}


void FindWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case M_QUIT_APP:
		{
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case M_FIND_RECIPE:
		{
			msg->AddString("name",fNameBox->Text());
			msg->AddString("category",fCategories->FindMarked()->Label());
			msg->AddString("ingredients",fIngredientBox->Text());
			fMessenger.SendMessage(msg);
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}
}

