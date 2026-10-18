#include "RecatWindow.h"

#include <Alert.h>
#include <StringView.h>
#include <ScrollView.h>
#include <Application.h>

#include "RecipeFuncs.h"

enum
{
	M_CHANGE_CAT = 'chct',
	M_QUIT_APP
};

RecatWindow::RecatWindow(const BRect &frame, const BMessenger &msgr,
						const int32 &number, const char *old)
 :	BWindow(frame,"Change Category",B_FLOATING_WINDOW_LOOK,
 			B_MODAL_SUBSET_WINDOW_FEEL,	B_ASYNCHRONOUS_CONTROLS),
 	fMessenger(msgr),
 	fNumber(number),
 	fOldCat(old)
{
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	AddShortcut('Q', B_COMMAND_KEY, new BMessage(M_QUIT_APP));
	
	BView *back = new BView(Bounds(),"back",B_FOLLOW_ALL, B_WILL_DRAW);
	back->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(back);
	
	BRect r;
	
	BString tempstr;
	tempstr << "Old Category: " << old;
	BStringView *oldlabel = new BStringView(BRect(10,10,11,11),"oldlabel",
											tempstr.String());
	oldlabel->ResizeToPreferred();
	back->AddChild(oldlabel);
	
	r = Bounds().InsetByCopy(10,10);
	r.bottom = oldlabel->Frame().bottom;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.OffsetBy(0,r.Height() + 10);
	BStringView *newlabel = new BStringView(r,"newlabel","New Category:");
	newlabel->ResizeToPreferred();
	back->AddChild(newlabel);
	
	r.OffsetBy(0,r.Height() + 5);
	r.bottom = 110;
	fNewList = new BListView(r, "newlist", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL);
	BScrollView *newscroll = new BScrollView("newscroll",fNewList,
											B_FOLLOW_LEFT | B_FOLLOW_TOP,
											0, false, true);
	back->AddChild(newscroll);
	
	fOK = new BButton(BRect(10,10,11,11),"ok","Cancel", new BMessage(M_CHANGE_CAT),
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
	
	fNewList->MakeFocus(true);
	
	r = fNewList->Parent()->Frame();
	r.bottom = fOK->Frame().top - 10;
	fNewList->Parent()->ResizeTo(r.Width(),r.Height());

	CppSQLite3Query query = DBQuery("select category from categories order by category;",
									"RecatWindow:GetCategories");
	while(!query.eof())
	{
		BString cat(DeescapeIllegalCharacters(query.getStringField(0)));
		fNewList->AddItem(new BStringItem(cat.String()));
		
		if(cat.ICompare(old)==0)
		{
			int32 index = fNewList->CountItems() - 1;
			fNewList->Select(index);
		}
		query.nextRow();
	}
}


void RecatWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case M_QUIT_APP:
		{
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case M_CHANGE_CAT:
		{
			BString newcat;
			int32 index = fNewList->CurrentSelection();
			
			if(index < 0)
			{
				BAlert *alert = new BAlert("Recibe","You need to select a new "
											"category", "OK");
				alert->Go();
				break;
			}
			
			newcat = ((BStringItem*)fNewList->ItemAt(index))->Text();	
			if(newcat != fOldCat)
			{
				msg->what = M_RECATEGORIZE_RECIPE;
				msg->AddInt32("number",fNumber);
				msg->AddString("oldcategory",fOldCat);
				msg->AddString("newcategory",newcat);
				fMessenger.SendMessage(msg);
			}
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}
}

