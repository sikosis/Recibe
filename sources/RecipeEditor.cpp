#include "RecipeEditor.h"
#include "RecipeFuncs.h"
#include <Application.h>
#include <Font.h>
#include <ScrollView.h>
#include <StringView.h>
#include <MenuField.h>
#include <MenuItem.h>

enum
{
	M_CATEGORY_CHANGED,
	M_QUIT_APP
};

RecipeEditor::RecipeEditor(const BRect &frame, const BMessenger &msgr,
						const int32 &number, const char *category)
 :	BWindow(frame,"Add Recipe",B_FLOATING_WINDOW_LOOK, B_MODAL_SUBSET_WINDOW_FEEL,
 			B_ASYNCHRONOUS_CONTROLS),
 	fMessenger(msgr),
 	fNumber(number),
 	fCategory(category)
{
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	AddShortcut('Q', B_COMMAND_KEY, new BMessage(M_QUIT_APP));
	
	BView *back = new BView(Bounds(),"back",B_FOLLOW_ALL, B_WILL_DRAW);
	back->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(back);
	
	fCategories = new BMenu("Categories");
	CppSQLite3Query query = DBQuery("select category from categories order by category;",
									"RecipeEditor:get categories");
	while(!query.eof())
	{
		BString cat(DeescapeIllegalCharacters(query.getStringField(0)));
		BMessage *menumsg = new BMessage(M_CATEGORY_CHANGED);
		menumsg->AddString("name",cat);
		fCategories->AddItem(new BMenuItem(cat.String(),menumsg));
		query.nextRow();
	}
	fCategories->SetRadioMode(true);
	fCategories->SetLabelFromMarked(true);
	if(fCategories->CountItems()>0)
		fCategories->ItemAt(0)->SetMarked(true);
	BRect r(10,10,10 + fCategories->MaxContentWidth(),11);
	BMenuField *field = new BMenuField(r,"field","Category",fCategories,
										B_FOLLOW_LEFT | B_FOLLOW_TOP,
										B_WILL_DRAW | B_NAVIGABLE | B_NAVIGABLE_JUMP);
	back->AddChild(field);
	field->ResizeToPreferred();
	r = field->Frame();
	
	if(category)
	{
		BMenuItem *marked = fCategories->FindItem(category);
		if(marked)
			marked->SetMarked(true);
	}
	else
	{
		BMenuItem *marked = fCategories->ItemAt(0L);
		if(marked)
			marked->SetMarked(true);
	}
	
	r.OffsetBy(0,r.Height() + 10);
	fNameBox = new AutoTextControl(r,"namebox","Name: ",NULL,NULL,
									B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
									B_WILL_DRAW | B_PULSE_NEEDED | B_NAVIGABLE |
									B_NAVIGABLE_JUMP);
	fNameBox->SetEscapeCancel(true);
	fNameBox->SetDivider(be_plain_font->StringWidth("Name ") + 5);
	back->AddChild(fNameBox);
	fNameBox->ResizeToPreferred();
	r = fNameBox->Frame();
	r.right = Bounds().right - 10;
	fNameBox->ResizeTo(r.Width(), r.Height());
	
	r.OffsetBy(0,r.Height() + 10);
	BStringView *label = new BStringView(r,"inglabel","Ingredients:");
	back->AddChild(label);
	
	r.OffsetBy(0,r.Height() + 10);
	r.bottom = r.top + 100;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	
	BRect textrect = r.OffsetToCopy(0,0);
	textrect.InsetBy(10,10);
	fIngredientBox = new BTextView(r, "ingredients", textrect, B_FOLLOW_ALL,
									B_WILL_DRAW | B_PULSE_NEEDED | B_NAVIGABLE |
									B_NAVIGABLE_JUMP);
	fIngredientBox->SetDoesUndo(true);
	
	
	BScrollView *ingredscroll = new BScrollView("ingredients_scroller",fIngredientBox,
										B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,0,false, true);
	back->AddChild(ingredscroll);
	
	r = ingredscroll->Frame();
	
	label = new BStringView(BRect(10,10,11,11),"dirlabel","Directions:");
	label->ResizeToPreferred();
	label->MoveTo(10, r.bottom + 10);
	back->AddChild(label);
	
	r.OffsetBy(0,r.Height() + 20 + label->Frame().Height());
	r.right -= B_V_SCROLL_BAR_WIDTH;
	textrect = r.OffsetToCopy(0,0);
	textrect.InsetBy(10,10);
	fDirectionsBox = new BTextView(r, "directions", textrect, B_FOLLOW_ALL,
									B_WILL_DRAW | B_PULSE_NEEDED | B_NAVIGABLE |
									B_NAVIGABLE_JUMP);
	fDirectionsBox->SetDoesUndo(true);
	
	BScrollView *dirscroll = new BScrollView("directions_scroller",fDirectionsBox,
										B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,0,false, true);
	back->AddChild(dirscroll);
	
	
	fOK = new BButton(BRect(10,10,11,11),"ok","Cancel", new BMessage(M_ADD_RECIPE),
							B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fOK->ResizeToPreferred();
	fOK->SetLabel("OK");
	fOK->MoveTo(Bounds().right - fOK->Bounds().Width() - 10,
				Bounds().bottom - fOK->Bounds().Height() - 10);
	r = fOK->Frame();
	back->AddChild(fOK);
	
	r.OffsetBy(-r.Width() - 10, 0);
	fCancel = new BButton(r,"cancel","Cancel",new BMessage(B_QUIT_REQUESTED),
						B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	back->AddChild(fCancel);
	
	field->MakeFocus(true);
	
	ResizeTo(Bounds().Width(),
			fDirectionsBox->Parent()->Frame().bottom + 20 + fOK->Bounds().Height());
	
	dirscroll->SetResizingMode(B_FOLLOW_ALL);
	
	// This is the part that's different for editing a recipe as opposed to
	// just adding one to the database
	if(number >= 0 && category)
	{
		SetTitle("Edit Recipe");
		BString name, ingredients, directions;
		if(GetRecipe(number,category,name,ingredients,directions))
		{
			BMenuItem *item = fCategories->FindItem(category);
			if(item)
				item->SetMarked(true);
			
			fNameBox->SetText(name.String());
			fIngredientBox->SetText(ingredients.String());
			fDirectionsBox->SetText(directions.String());
			fOK->SetMessage(new BMessage(M_EDIT_RECIPE));
		}
	}
	
	AddShortcut(B_ENTER, B_COMMAND_KEY, new BMessage(fOK->Command()));
}


void RecipeEditor::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case M_QUIT_APP:
		{
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case M_CATEGORY_CHANGED:
		{
			BString name;
			if(msg->FindString("name",&name)==B_OK)
				fCategory = name;
			break;
		}
		case M_EDIT_RECIPE:
		{
			BMenuItem *item = fCategories->FindMarked();
			if(fCategory.Compare(item->Label()) == 0)
			{
				UpdateRecipe(fNumber, fCategory.String(), fNameBox->Text(),
						fIngredientBox->Text(), fDirectionsBox->Text());
			}
			else
			{
				// we're also recategorizing the recipe, so we'll just delete the old
				// one and add it to the new one
				DeleteRecipe(fNumber, fCategory.String());
				AddRecipe(fCategory.String(), fNameBox->Text(),
						fIngredientBox->Text(), fDirectionsBox->Text());
			}
			fMessenger.SendMessage(M_SET_RECIPE);
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case M_ADD_RECIPE:
		{
			if(fCategory.CountChars() < 1)
			{
				BMenuItem *item = fCategories->FindMarked();
				if(item)
					fCategory = item->Label();
				else
					fCategory = "Misc";
			}
			
			AddRecipe(fCategory.String(), fNameBox->Text(),
					fIngredientBox->Text(), fDirectionsBox->Text());
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}
}

void RecipeEditor::FrameResized(float w, float h)
{
	fIngredientBox->SetTextRect(fIngredientBox->Bounds().InsetByCopy(10,10));
	fDirectionsBox->SetTextRect(fDirectionsBox->Bounds().InsetByCopy(10,10));
}
