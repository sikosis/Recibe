#include "ChefWindow.h"
#include "ChefView.h"
#include <Application.h>

ChefWindow::ChefWindow(const BRect &rect)
 : BWindow(rect,"Recibe",B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	fView = new ChefView(Bounds());
	AddChild(fView);
	
	SetSizeLimits(rect.Width(),30000,rect.Height(),30000);
}


bool ChefWindow::QuitRequested(void)
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void ChefWindow::MessageReceived(BMessage *msg)
{
	if(msg->what == M_JUMP_TO_BOOKMARK)
		fView->MessageReceived(msg);
	else
		BWindow::MessageReceived(msg);
}
