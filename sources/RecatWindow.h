#ifndef RECATWINDOW_H
#define RECATWINDOW_H

#include <Window.h>
#include <View.h>
#include <Messenger.h>
#include <Button.h>
#include <ListView.h>
#include <String.h>

#define M_RECATEGORIZE_RECIPE 'rcrc'

class RecatWindow : public BWindow
{
public:
	RecatWindow(const BRect &frame, const BMessenger &msgr, const int32 &number,
				const char *old);
	void MessageReceived(BMessage *msg);

private:
	BMessenger fMessenger;
	
	BListView	*fNewList;

	BButton		*fOK,
				*fCancel;
	
	int32		fNumber;
	BString		fOldCat;
};

#endif
