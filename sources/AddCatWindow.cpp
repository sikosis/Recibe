#include "AddCatWindow.h"
#include "RecipeFuncs.h"
#include <Application.h>
#include <Font.h>
#include <MenuField.h>
#include <MenuItem.h>

enum
{
	M_QUIT_APP='quap'
};

AddCatWindow::AddCatWindow(const BRect &frame, const BMessenger &msgr)
 :	BWindow(frame,"Add Category",B_FLOATING_WINDOW_LOOK, B_MODAL_SUBSET_WINDOW_FEEL,
 			B_ASYNCHRONOUS_CONTROLS),
 	fMessenger(msgr)
{
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	AddShortcut('Q', B_COMMAND_KEY, new BMessage(M_QUIT_APP));
	
	BView *back = new BView(Bounds(),"back",B_FOLLOW_ALL, B_WILL_DRAW);
	back->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(back);
	
	BRect r(Bounds().InsetByCopy(10,10));
	float namewidth = be_plain_font->StringWidth("New Category Name: ") + 5;
	fNameBox = new AutoTextControl(r,"namebox","New Category Name: ","", NULL,
									B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fNameBox->SetDivider(namewidth);
	fNameBox->SetEscapeCancel(true);
	back->AddChild(fNameBox);
	fNameBox->ResizeToPreferred();
	r = fNameBox->Frame();
	r.right = Bounds().right - 10;
	fNameBox->ResizeTo(r.Width(), r.Height());
	
	fOK = new BButton(BRect(10,10,11,11),"ok","Cancel", new BMessage(M_ADD_CATEGORY),
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
	
	fNameBox->MakeFocus(true);
	
	ResizeTo((namewidth * 2) + 20,
			fNameBox->Frame().bottom + 20 + fOK->Bounds().Height());
}


void AddCatWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case M_QUIT_APP:
		{
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case M_ADD_CATEGORY:
		{
			if(strlen(fNameBox->Text()))
			{
				msg->AddString("name",fNameBox->Text());
				fMessenger.SendMessage(msg);
				PostMessage(B_QUIT_REQUESTED);
			}
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}
}

