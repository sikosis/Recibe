#include "DelCatWindow.h"
#include "RecipeFuncs.h"
#include <Application.h>
#include <Font.h>
#include <MenuField.h>
#include <MenuItem.h>

enum
{
	M_CONFIRM_CHANGED,
	M_DELETE_CATEGORY,
	M_QUIT_APP
};

DelCatWindow::DelCatWindow(const BRect &frame, const char *category)
 :	BWindow(frame,"Delete Category",B_FLOATING_WINDOW_LOOK, B_MODAL_SUBSET_WINDOW_FEEL,
 			B_ASYNCHRONOUS_CONTROLS)
{
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	AddShortcut('Q', B_COMMAND_KEY, new BMessage(M_QUIT_APP));
	
	BView *back = new BView(Bounds(),"back",B_FOLLOW_ALL, B_WILL_DRAW);
	back->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(back);
	
	BRect r;
	
	r = Bounds().InsetByCopy(10,10);
	fWarningLabel = new BTextView(r,"warninglabel",r.OffsetToCopy(0,0),
								B_FOLLOW_ALL, B_WILL_DRAW);
	back->AddChild(fWarningLabel);
	fWarningLabel->MakeEditable(false);
	fWarningLabel->MakeSelectable(false);
	fWarningLabel->SetStylable(true);
	fWarningLabel->SetViewColor(back->ViewColor());
	
	fWarningLabel->SetFontAndColor(be_bold_font);
	fWarningLabel->Insert("WARNING:\n\n");
	fWarningLabel->SetFontAndColor(be_plain_font);
	fWarningLabel->Insert("This will delete ALL recipes in the category "
						"and cannot be undone. Please type the word delete "
						"into the box to confirm.");
	fWarningLabel->SetTextRect(fWarningLabel->Bounds());
	fWarningLabel->ResizeTo(Bounds().Width() - 20,
			(fWarningLabel->LineHeight() * fWarningLabel->CountLines())+10);
	fWarningLabel->SetTextRect(fWarningLabel->Bounds());
	
	
	fCategories = new BMenu("Categories");
	CppSQLite3Query query = DBQuery("select category from categories order by category;",
									"DelCatWindow:get categories");
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
	r.Set(10,10,10 + fCategories->MaxContentWidth(),11);
	r.OffsetTo(10, fWarningLabel->Frame().bottom + 10);
	BMenuField *field = new BMenuField(r,"field","Category",fCategories);
	back->AddChild(field);
	field->ResizeToPreferred();
	
	BMenuItem *marked = fCategories->FindItem(category);
	if(marked)
		marked->SetMarked(true);
	
	r = field->Frame();
	r.OffsetTo(0,field->Frame().bottom + 10);
	fConfirmBox = new AutoTextControl(r,"confirmbox","","",
									new BMessage(M_CONFIRM_CHANGED),
									B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fConfirmBox->SetEscapeCancel(true);
	back->AddChild(fConfirmBox);
	
	fConfirmBox->SetDivider(0);
	fConfirmBox->ResizeToPreferred();
	
	r = fConfirmBox->Frame();
	r.right = Bounds().right - 10;
	fConfirmBox->ResizeTo(r.Width(), r.Height());
	
	fOK = new BButton(BRect(10,10,11,11),"ok","Cancel", new BMessage(M_DELETE_CATEGORY),
					B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fOK->ResizeToPreferred();
	fOK->SetLabel("OK");
	fOK->MoveTo(Bounds().right - fOK->Bounds().Width() - 10,
				Bounds().bottom - fOK->Bounds().Height() - 10);
	r = fOK->Frame();
	back->AddChild(fOK);
	fOK->MakeDefault(true);
	fOK->SetEnabled(false);
	
	r.OffsetBy(-r.Width() - 10, 0);
	fCancel = new BButton(r,"cancel","Cancel",new BMessage(B_QUIT_REQUESTED),
						B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	back->AddChild(fCancel);
	
	if(category)
		fConfirmBox->MakeFocus();
	else
		field->MakeFocus(true);
	
	ResizeTo(Bounds().Width(),
			fConfirmBox->Frame().bottom + 20 + fOK->Bounds().Height());
}


void DelCatWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case M_QUIT_APP:
		{
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case M_CONFIRM_CHANGED:
		{
			BString text = fConfirmBox->Text();
			if(text.IFindFirst("delete")!=B_ERROR)
				fOK->SetEnabled(true);
			else
				fOK->SetEnabled(false);
			break;
		}
		case M_DELETE_CATEGORY:
		{
			BMenuItem *item = fCategories->FindMarked();
			if(!item)
				break;
			BString cat = EscapeIllegalCharacters(item->Label());
			BString command = "drop table ";
			command << cat << ";";
			DBCommand(command.String(),"DelCatWindow:delete category table");
			
			command = "delete from categories where category = '";
			command << cat << "';";
			DBCommand(command.String(), "DelCatWindow:delete category from cat list");
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}
}

