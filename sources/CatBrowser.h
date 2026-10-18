#ifndef CATBROWSER_H
#define CATBROWSER_H

#include <Window.h>
#include <View.h>
#include <Menu.h>
#include <Messenger.h>
#include <Button.h>
#include <ListView.h>
#include <String.h>

#define M_LOOKUP_RECIPE 'lkrc'

class CatBrowser : public BWindow
{
public:
	CatBrowser(const BRect &frame, const BMessenger &msgr, const char *category,
				bool editmode = false);
	~CatBrowser(void);
	void MessageReceived(BMessage *msg);

private:
	void RunQuery(void);
	void SetupQuery(void);
	
	BMessenger fMessenger;
	
	BListView	*fList;

	BMenu		*fCategories;
	
	BButton		*fClose,
				*fOpen,
				*fBack,
				*fNext;
	
	BString		fCategory;
	int32		fCategoryCount;
	
	BList		fPageList;
	int32		fCurrentPage;
	int32		fPageCount;
	
	bool		fEditMode;
	BString		fSortString;
};

#endif
