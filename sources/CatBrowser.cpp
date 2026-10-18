#include "CatBrowser.h"

#include <Alert.h>
#include <StringView.h>
#include <ScrollView.h>
#include <Application.h>
#include <MenuItem.h>
#include <MenuField.h>
#include "FindWindow.h"
#include "RecatWindow.h"
#include "RecipeEditor.h"
#include "RecipeFuncs.h"
#include "ChefView.h"

#define EDIT_MODE_RECIPE_COUNT 600
#define BROWSE_MODE_RECIPE_COUNT 200

enum
{
	M_QUIT_APP,
	M_SET_CATEGORY,
	M_RESULTS_BACK,
	M_RESULTS_NEXT,
	
	// edit mode functions
	M_DELETE_RECIPE,
	M_SHOW_RECAT
};

class QueryPage
{
public:
	QueryPage(void)
	{
		start = 0;
		end = 0;
		init = false;
	}
	void MakeEmpty(void)
	{
		start = 0;
		end = 0;
		init = false;
	}
	
	int32 start;
	int32 end;
	bool init;
};

CatBrowser::CatBrowser(const BRect &frame, const BMessenger &msgr,
						const char *category, bool editmode)
 :	BWindow(frame,"Recipe Browser",B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS),
 	fMessenger(msgr),
 	fCategoryCount(-1),
 	fCurrentPage(-1),
 	fEditMode(editmode)
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
		fCategories->AddItem(new BMenuItem(cat.String(),new BMessage(M_SET_CATEGORY)));
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
	r.right = Bounds().right - 10 - B_V_SCROLL_BAR_WIDTH;
	r.bottom = Bounds().bottom - 10;
	
	fList = new BListView(r, "newlist", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL);
	fList->SetInvocationMessage(new BMessage(M_SET_RECIPE));
	BScrollView *newscroll = new BScrollView("newscroll",fList,
											B_FOLLOW_ALL, 0, false, true);
	back->AddChild(newscroll);
	
	fBack = new BButton(BRect(10,10,11,11),"back","Back", new BMessage(M_RESULTS_BACK),
					B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fBack->ResizeToPreferred();
	fBack->ResizeTo(be_plain_font->StringWidth("Back") + 15,fBack->Frame().Height());
	fBack->MoveTo(10,Bounds().bottom - fBack->Bounds().Height() - 10);
	r = fBack->Frame();
	back->AddChild(fBack);
	fBack->SetEnabled(false);
	
	r.OffsetBy(r.Width() + 5, 0);
	fNext = new BButton(r,"next","Next", new BMessage(M_RESULTS_NEXT),
					B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	back->AddChild(fNext);
	fNext->SetEnabled(false);
	
	fClose = new BButton(BRect(10,10,11,11),"close","View Recipe", new BMessage(B_QUIT_REQUESTED),
					B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fClose->ResizeToPreferred();
	fClose->SetLabel("Close");
	fClose->MoveTo(Bounds().right - fClose->Bounds().Width() - 10,r.top);
	r = fClose->Frame();
	fClose->MakeDefault(true);
	
	r.OffsetBy(-r.Width() - 10, 0);
	fOpen = new BButton(r,"openrecipe","View Recipe",new BMessage(M_SET_RECIPE),
						B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	
	back->AddChild(fOpen);
	back->AddChild(fClose);
	
	fList->MakeFocus(true);
	
	r = fList->Parent()->Frame();
	r.bottom = fClose->Frame().top - 10;
	fList->Parent()->ResizeTo(r.Width(),r.Height());
	
	float controlwidth = (fBack->Frame().Width() * 2) + (fOpen->Frame().Width() * 2) + 65;
	if( Frame().Width() < controlwidth )
		ResizeTo(controlwidth, Frame().Height());
	
	if(fEditMode)
	{
		// This hidden mode (Command-Click on the browser menu item) is for doing
		// massive edits on the database
		fList->SetListType(B_MULTIPLE_SELECTION_LIST);
		fList->SetSelectionMessage(new BMessage(M_SET_RECIPE));
		
		BMenu *editmenu = new BMenu("Edit");
		editmenu->AddItem(new BMenuItem("Mass Recategorize",new BMessage(M_SHOW_RECAT)));
		editmenu->AddItem(new BMenuItem("Delete Recipe",new BMessage(M_DELETE_RECIPE)));
		
		r.Set(10,10,10 + be_plain_font->StringWidth("Edit") + 25,11);
		r.OffsetTo(Bounds().right - 10 - r.Width(), 10);
		BMenuField *editfield = new BMenuField(r,"editfield","Edit",editmenu,
											B_FOLLOW_RIGHT | B_FOLLOW_TOP);
		editfield->SetDivider(0);
		back->AddChild(editfield);
		
		fSortString = " order by number ";
	}
	
	PostMessage(M_SET_CATEGORY);
}

CatBrowser::~CatBrowser(void)
{
	for(int32 i = 0; i < fPageList.CountItems(); i++)
	{
		QueryPage *page = (QueryPage*)fPageList.ItemAt(i);
		delete page;
	}
}

void CatBrowser::SetupQuery(void)
{
	BString command, esccat = EscapeIllegalCharacters(fCategory.String());
	CppSQLite3Query query;
	
	command = "select count(number) from ";
	command << esccat << ";";
	query = DBQuery(command.String(), "CatBrowser::SetupQuery:get category count");
	
	fCategoryCount = (query.eof()) ? 0 : query.getIntField(0);
	
	for(int32 i = 0; i < fPageList.CountItems(); i++)
	{
		QueryPage *page = (QueryPage*)fPageList.ItemAt(i);
		page->MakeEmpty();
	}
	
	fPageCount = (fCategoryCount / BROWSE_MODE_RECIPE_COUNT);
	if( (fPageCount % BROWSE_MODE_RECIPE_COUNT) || fPageCount < BROWSE_MODE_RECIPE_COUNT)
		fPageCount++;
	if(fPageList.CountItems() < fPageCount)
	{
		for(int32 i = fPageList.CountItems(); i <= fPageCount; i++)
			fPageList.AddItem(new QueryPage());
	}
	
	fCurrentPage = -1;
}

void CatBrowser::RunQuery(void)
{
	BString esccat(EscapeIllegalCharacters(fCategory.String()));
	if(fCategory.CountChars() < 1)
		return;
	
	fList->DeselectAll();
	for(int32 i = 0; i < fList->CountItems(); i++)
	{
		RecipeItem *item = (RecipeItem*)fList->ItemAt(i);
		delete item;
	}
	fList->MakeEmpty();
	
	if(!fCategoryCount)
		return;
	
	
	int32 itemcount = fEditMode ? EDIT_MODE_RECIPE_COUNT : BROWSE_MODE_RECIPE_COUNT;
	
	BString command;
	CppSQLite3Query query;
	
	if(fCurrentPage >= 0)
	{
		QueryPage *page = (QueryPage*)fPageList.ItemAt(fCurrentPage);
		QueryPage *lastpage = NULL;
		
		if(!page)
			return;
		
		command = "select * from ";
		if(page->init)
		{
			command << esccat << " where (number >= " << page->start
					<< ") and (number <= " << page->end << ") " << fSortString << ";";
		}
		else
		{
			lastpage = (QueryPage*)fPageList.ItemAt(fCurrentPage - 1);
			command << esccat << " where number >= " << (lastpage->end + 1)
					<< fSortString << ";";
		}
		
		query = DBQuery(command.String(), "CatBrowser:get category");
		RecipeItem *ritem = NULL;
		for(int32 i = 0; i < itemcount; i++)
		{
			if(query.eof())
				break;
			
			BString string = query.getStringField(1);
			ritem = new RecipeItem(DeescapeIllegalCharacters(string.String()).String(),
									esccat.String(), query.getIntField(0));
			fList->AddItem(ritem);
			query.nextRow();
		}
		if(lastpage && ritem)
		{
			page->start = lastpage->end + 1;
			page->end = ritem->fNumber;
			page->init = true;
		}
	}
	else
	{
		command = "select * from ";
		command << esccat << " where number >= 0 " << fSortString << ";";
		
		query = DBQuery(command.String(), "CatBrowser:get category");
		
		RecipeItem *ritem = NULL;
		for(int32 i = 0; i < itemcount; i++)
		{
			if(query.eof())
				break;
			
			BString string = query.getStringField(1);
			ritem = new RecipeItem(DeescapeIllegalCharacters(string.String()).String(),
									esccat.String(), query.getIntField(0));
			fList->AddItem(ritem);
			query.nextRow();
		}
		if(ritem)
		{
			fCurrentPage = 0;
			QueryPage *page = (QueryPage*)fPageList.ItemAt(fCurrentPage);
			page->start = 0;
			page->end = ritem->fNumber;
			page->init = true;
		}
	}
	
	fList->Select(0L);
	fList->MakeFocus(true);
}

void CatBrowser::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case M_QUIT_APP:
		{
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case M_SET_CATEGORY:
		{
			BMenuItem *marked = fCategories->FindMarked();
			if(!marked)
				break;
			else
				fCategory = marked->Label();
			fBack->SetEnabled(false);
			SetupQuery();
			RunQuery();
			
			if(fPageCount < 2)
				fNext->SetEnabled(false);
			else
				fNext->SetEnabled(true);
			break;
		}
		case M_RESULTS_BACK:
		{
			if(fCurrentPage > 0)
			{
				fCurrentPage--;
				RunQuery();
			}
			if(fCurrentPage < fPageCount - 1)
				fNext->SetEnabled(true);
			
			if(fCurrentPage == 0)
				fBack->SetEnabled(false);
			break;
		}
		case M_RESULTS_NEXT:
		{
			if(fCurrentPage < fPageCount - 1)
			{
				fCurrentPage++;
				if(fCurrentPage >= fPageCount - 1)
					fNext->SetEnabled(false);
				
				RunQuery();
			}
			
			if(!fBack->IsEnabled())
				fBack->SetEnabled(true);
			break;
		}
		case M_RECATEGORIZE_RECIPE:
		{
			int32 number;
			BString oldcat, newcat;
			msg->FindInt32("number",&number);
			msg->FindString("oldcategory",&oldcat);
			msg->FindString("newcategory",&newcat);
			
			int32 index = 0, selection;
			DBCommand("BEGIN","CatBrowser:begin mass recat");
			BeginViewTransaction();
			do
			{
				selection = fList->CurrentSelection(index);
				index++;
				if(selection >=0)
				{
					RecipeItem *item = (RecipeItem*)fList->ItemAt(selection);
					if(item)
					{
						ChangeCategory(item->fNumber, item->fCategory.String(),
										newcat.String());
					}
				}
				
			} while(selection >= 0);
			EndViewTransaction();
			DBCommand("COMMIT","ChefView:end mass recat");
			SetupQuery();
			RunQuery();
			break;
		}
		case M_SHOW_RECAT:
		{
			int32 selection, firstselection, count=0;
			firstselection = selection = fList->CurrentSelection();
			if(selection < 0)
				break;
			count++;
			
			do
			{
				selection = fList->CurrentSelection(count);
				if(selection >= 0)
					count++;
			} while (selection >= 0);
			
			RecipeItem *item = (RecipeItem*)fList->ItemAt(firstselection);
			if(item)
			{
				BRect r(Frame().OffsetByCopy(20,20));
				r.right = r.left + 300;
				r.bottom = r.top + 200;
				RecatWindow *win = new RecatWindow(r,BMessenger(this), 
												(count>1) ? -1 : item->fNumber,
												item->fCategory.String());
				win->AddToSubset(this);
				win->Show();
			}
			break;
		}
		case M_DELETE_RECIPE:
		{
			int32 index = 0, selection;
			DBCommand("BEGIN","CatBrowser:begin mass delete");
			BeginViewTransaction();
			do
			{
				selection = fList->CurrentSelection(index);
				index++;
				if(selection >=0)
				{
					RecipeItem *item = (RecipeItem*)fList->ItemAt(selection);
					if(item)
						DeleteRecipe(item->fNumber, item->fCategory.String());
				}
				
			} while(selection >= 0);
			EndViewTransaction();
			DBCommand("COMMIT","ChefView:end mass recat");
			SetupQuery();
			RunQuery();
			
			// refresh the display in the main window in case we deleted something
			// that would be in the results
			BMessage msg(M_FIND_RECIPE);
			fMessenger.SendMessage(&msg);
			break;
		}
		case M_SET_RECIPE:
		{
			if(fEditMode)
			{
				if(fList->CurrentSelection(1)>=0)
					break;
			}
			
			RecipeItem *item = (RecipeItem*)fList->ItemAt(fList->CurrentSelection());
			if(item)
			{
				BMessage msg(M_LOOKUP_RECIPE);
				msg.AddInt32("number",item->fNumber);
				msg.AddString("category",item->fCategory);
				msg.AddString("name",item->fName);
				fMessenger.SendMessage(&msg);
			}
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}
}

