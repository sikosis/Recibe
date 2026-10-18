#include "ChefView.h"
#include <Alert.h>
#include <Path.h>
#include <Roster.h>
#include <Application.h>
#include <stdlib.h>
#include <Messenger.h>
#include <ScrollView.h>
#include <PrintJob.h>
#include <MenuBar.h>
#include <stdlib.h>
#include <MenuItem.h>
#include <ListItem.h>
#include <TranslationUtils.h>
#include <FindDirectory.h>
#include <locale.h>
#include <string.h>

#include "AboutWindow.h"
#include "DelCatWindow.h"
#include "FindWindow.h"
#include "RecipeFuncs.h"
#include "RecipeEditor.h"
#include "CatBrowser.h"
#include "RecatWindow.h"
#include "AddCatWindow.h"
#include "TextFile.h"
#include "SplitterView.h"

enum
{
	M_SHOW_FIND_WINDOW='sfwn',
	
	M_SHOW_RECAT_WINDOW,
	M_SHOW_MASS_RECAT_WINDOW,
	M_SHOW_ADDCAT_WINDOW,
	M_SHOW_DELCAT_WINDOW,
	M_SHOW_ADD_RECIPE,
	M_SHOW_EDIT_RECIPE,
	M_SHOW_EXPORT_RECIPE,
	M_SHOW_BROWSER,
	M_SHOW_EMAIL_WINDOW,
	
	M_IMPORT_MEALMASTER,
	M_EXPORT_MEALMASTER,
	
	M_EXPORT_RECIPE,
	M_EMAIL_RECIPE,
	M_PRINT_RECIPE,
	M_DELETE_RECIPE,
	
	M_ADD_BOOKMARK,
	M_DELETE_BOOKMARK,
	
	M_DELETE_CATEGORY,
	M_SHOW_ABOUT
};

static bool
UsesBritishSpelling()
{
	setlocale(LC_ALL, "");
	const char* language = setlocale(LC_MESSAGES, NULL);
	if (language == NULL)
		return false;

	return strncmp(language, "en_AU", 5) == 0
		|| strncmp(language, "en-AU", 5) == 0
		|| strncmp(language, "en_GB", 5) == 0
		|| strncmp(language, "en-GB", 5) == 0
		|| strncmp(language, "en_UK", 5) == 0
		|| strncmp(language, "en-UK", 5) == 0;
}

RecipeItem::RecipeItem(const char *name, const char *category, const int32 &number)
 :	BStringItem((const char *)NULL),
 	fName(name),
 	fCategory(category),
 	fNumber(number)
{
	BString title;
	title << number << ": " << name;
	SetText(title.String());
}

class RecipeMenuItem : public BMenuItem
{
public:
	RecipeMenuItem(const char *name, const char *category, const int32 &number,
					const int32 &what);
	
	BString fName, fCategory;
	int32 fNumber;
};

RecipeMenuItem::RecipeMenuItem(const char *name, const char *category,
								const int32 &number, const int32 &what)
 :	BMenuItem(name,NULL),
 	fName(name),
 	fCategory(category),
 	fNumber(number)
{
	BMessage *msg = new BMessage(what);
	msg->AddString("name",name);
	msg->AddString("category",category);
	msg->AddInt32("number",number);
	SetMessage(msg);
}

ChefView::ChefView(const BRect &rect)
 :	BView(rect, "chefview", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS),
 	fPrintSettings(NULL),
 	fHeaderFont(be_bold_font)
{
	
	fHeaderFont.SetSize(be_plain_font->Size() + 2);
	
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	BString storePath(path.Path());
	storePath << "/Recibe/recipes";
	
	OpenRecipeStore(storePath.String());
	
	BView *back = new BView(Bounds(),"back",B_FOLLOW_ALL, B_WILL_DRAW);
	back->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(back);
	
	BRect r(Bounds());
	font_height fh;
	be_plain_font->GetHeight(&fh);
	r.bottom = fh.ascent + fh.descent + fh.leading + 5;
	BMenuBar *bar = new BMenuBar(r,"menubar");
	back->AddChild(bar);
	
	fFileMenu = new BMenu("File");
	fFileMenu->AddItem(new BMenuItem("Export Recipe…",
		new BMessage(M_SHOW_EXPORT_RECIPE), 'S', B_COMMAND_KEY | B_SHIFT_KEY));
	fFileMenu->AddItem(new BMenuItem("Print…",new BMessage(M_PRINT_RECIPE),'P'));
	fFileMenu->AddSeparatorItem();
	fFileMenu->AddItem(new BMenuItem("About Recibe…",new BMessage(M_SHOW_ABOUT)));
	fFileMenu->AddSeparatorItem();
	fFileMenu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
	bar->AddItem(fFileMenu);
	
	fRecipeMenu = new BMenu("Recipe");
	fRecipeMenu->AddItem(new BMenuItem("Find…",new BMessage(M_SHOW_FIND_WINDOW),'F',B_COMMAND_KEY));
	fRecipeMenu->AddItem(new BMenuItem("Show Recipe Browser…",new BMessage(M_SHOW_BROWSER),'B'));
	fRecipeMenu->AddSeparatorItem();
	fRecipeMenu->AddItem(new BMenuItem("Edit…",new BMessage(M_SHOW_EDIT_RECIPE),'E'));
	fRecipeMenu->AddItem(new BMenuItem("Change Category…",new BMessage(M_SHOW_RECAT_WINDOW)));
	fRecipeMenu->AddSeparatorItem();
	fRecipeMenu->AddItem(new BMenuItem("Add…",new BMessage(M_SHOW_ADD_RECIPE),'R'));
	fRecipeMenu->AddItem(new BMenuItem("Delete…",new BMessage(M_DELETE_RECIPE)));
	fRecipeMenu->AddSeparatorItem();
	fRecipeMenu->AddItem(new BMenuItem("Add Category…",new BMessage(M_SHOW_ADDCAT_WINDOW)));
	fRecipeMenu->AddItem(new BMenuItem("Delete Category…",new BMessage(M_SHOW_DELCAT_WINDOW)));
	bar->AddItem(fRecipeMenu);
	
//	fEmailMenu = new BMenu("E-Mail");
//	fEmailMenu->AddItem(new BMenuItem("Show E-Mail Window",new BMessage(M_SHOW_EMAIL_WINDOW)));
//	bar->AddItem(fEmailMenu);
	
	bool britishSpelling = UsesBritishSpelling();
	fFavoritesMenu = new BMenu(britishSpelling ? "Favourites" : "Favorites");
	fFavoritesMenu->AddItem(new BMenuItem(britishSpelling
		? "Add to Favourites" : "Add to Favorites", new BMessage(M_ADD_BOOKMARK)));
	fFavoritesMenu->AddItem(new BMenuItem(britishSpelling
		? "Remove from Favourites" : "Remove from Favorites",
		new BMessage(M_DELETE_BOOKMARK)));
	bar->AddItem(fFavoritesMenu);
	
	r.OffsetBy(10,r.Height() + 5);
	fCategoryLabel = new BStringView(r,"category","Category: [None]",B_FOLLOW_LEFT | B_FOLLOW_TOP);
	back->AddChild(fCategoryLabel);
	
	r = fCategoryLabel->Frame();
	r.OffsetBy(0,r.Height() + 20);
	fFind = new BButton(r, "find", "Find…", new BMessage(M_SHOW_FIND_WINDOW),
						B_FOLLOW_LEFT | B_FOLLOW_TOP);
	fFind->ResizeToPreferred();
	r = fFind->Frame();
	back->AddChild(fFind);
	
	r.OffsetBy(0,r.Height() + 10);
	r.bottom = Bounds().bottom - 10;
	r.right = r.right + 100 - B_V_SCROLL_BAR_WIDTH;
//	fResults = new BListView(r, "results",B_SINGLE_SELECTION_LIST,B_FOLLOW_ALL);
	fResults = new BListView(r, "results",B_MULTIPLE_SELECTION_LIST,B_FOLLOW_ALL);
	fResults->SetSelectionMessage(new BMessage(M_SET_RECIPE));
	BScrollView *scroll = new BScrollView("scrollresults",fResults,
										B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM,0,false, true);
	back->AddChild(scroll);
	
	fCategoryLabel->ResizeTo(fResults->Frame().Width(),fCategoryLabel->Frame().Height());
	
	r.left = fResults->Parent()->Frame().right + 20;
	r.top = bar->Frame().bottom + 10;
	r.right = Bounds().right - 10 - B_V_SCROLL_BAR_WIDTH;
	r.bottom = Bounds().bottom - 10;
	
	BRect textrect = r.OffsetToCopy(0,0);
	textrect.InsetBy(10,10);
	fRecipe = new BTextView(r, "recipe", textrect, B_FOLLOW_ALL);
	scroll = new BScrollView("scrollrecipe",fRecipe,B_FOLLOW_ALL,0,false, true);
	back->AddChild(scroll);
	
	fRecipe->MakeEditable(false);
	fRecipe->SetStylable(true);
	
	fExportPanel = new BFilePanel(B_SAVE_PANEL, NULL, NULL, 0, false,
		new BMessage(M_EXPORT_RECIPE));
	
	LoadFavorites();
}

ChefView::~ChefView(void)
{
	SaveFavorites();
	
	CloseRecipeStore();
	delete fExportPanel;
}

void ChefView::AttachedToWindow(void)
{
	Window()->AddShortcut(B_DELETE, B_COMMAND_KEY, new BMessage(M_DELETE_RECIPE),this);

	fFind->SetTarget(this);
	fResults->SetTarget(this);
	
	BMessenger msgr(this);
	fExportPanel->SetTarget(msgr);
	
	fFileMenu->SetTargetForItems(this);
	fRecipeMenu->SetTargetForItems(this);
	fFavoritesMenu->SetTargetForItems(this);
}

void ChefView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case B_QUIT_REQUESTED:
		{
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		}
//		case M_SHOW_EMAIL_WINDOW:
//		{
//			break;
//		}
		case M_SHOW_ABOUT:
		{
			AboutWindow *win = new AboutWindow();
			win->Show();
			break;
		}
		case M_SHOW_BROWSER:
		{
			bool editmode = (modifiers() & B_OPTION_KEY);
			BRect r(Window()->Frame().OffsetByCopy(20,20));
			r.right = r.left + 300;
			r.bottom = r.top + 300;
			CatBrowser *win = new CatBrowser(r,BMessenger(this), fSearchCategory.String(),
											editmode);
			win->Show();
			break;
		}
		case M_SHOW_EXPORT_RECIPE:
		{
			RecipeItem* item = GetCurrentRecipe();
			if(item == NULL)
				break;
			fExportPanel->SetSaveText(item->fName.String());
			if(fExportPanel->IsShowing())
				fExportPanel->Window()->Activate();
			fExportPanel->Show();
			break;
		}
		case M_SHOW_FIND_WINDOW:
		{
			BRect r(Window()->Frame().OffsetByCopy(20,20));
			r.right = r.left + 300;
			r.bottom = r.top + 200;
			FindWindow *win = new FindWindow(r,BMessenger(this), fSearchName.String(),
											fSearchCategory.String(),
											fSearchIngredients.String());
			win->AddToSubset(Window());
			win->Show();
			break;
		}
		case M_SHOW_RECAT_WINDOW:
		{
			int32 selection, firstselection, count=0;
			firstselection = selection = fResults->CurrentSelection();
			if(selection < 0)
				break;
			count++;
			
			do
			{
				selection = fResults->CurrentSelection(count);
				if(selection >= 0)
					count++;
			} while (selection >= 0);
			
			RecipeItem *item = (RecipeItem*)fResults->ItemAt(firstselection);
			if(item)
			{
				BRect r(Window()->Frame().OffsetByCopy(20,20));
				r.right = r.left + 300;
				r.bottom = r.top + 200;
				RecatWindow *win = new RecatWindow(r,BMessenger(this), 
												(count>1) ? -1 : item->fNumber,
												item->fCategory.String());
				win->AddToSubset(Window());
				win->Show();
			}
			break;
		}
		case M_SHOW_ADD_RECIPE:
		{
			BRect r(Window()->Frame().OffsetByCopy(20,20));
			r.right = r.left + 300;
			r.bottom = r.top + 200;
			RecipeEditor *win = new RecipeEditor(r,BMessenger(this),-1,NULL);
			win->AddToSubset(Window());
			win->Show();
			break;
		}
		case M_SHOW_EDIT_RECIPE:
		{
			RecipeItem *item = GetCurrentRecipe();
			if(item)
			{
				// If we have multiple items selected when we go to edit a recipe,
				// it could be confusing to the user, so we will deselect all items
				// except the first one, which is also the one to be edited.
				if(fResults->CurrentSelection(1) != -1)
					fResults->Select(fResults->IndexOf(item));
				
				BRect r(Window()->Frame().OffsetByCopy(20,20));
				r.right = r.left + 300;
				r.bottom = r.top + 200;
				RecipeEditor *win = new RecipeEditor(r,BMessenger(this),item->fNumber,
													item->fCategory.String());
				win->AddToSubset(Window());
				win->Show();
			}
			break;
		}
		case M_SHOW_ADDCAT_WINDOW:
		{
			BRect r(Window()->Frame().OffsetByCopy(20,20));
			r.right = r.left + 300;
			r.bottom = r.top + 200;
			AddCatWindow *win = new AddCatWindow(r,BMessenger(this));
			win->AddToSubset(Window());
			win->Show();
			break;
		}
		case M_SHOW_DELCAT_WINDOW:
		{
			BString string;
			RecipeItem *item = GetCurrentRecipe();
			if(item)
				string = item->fCategory;
			
			BRect r(Window()->Frame().OffsetByCopy(20,20));
			r.right = r.left + 300;
			r.bottom = r.top + 200;
			DelCatWindow *win = new DelCatWindow(r,string.String());
			win->AddToSubset(Window());
			win->Show();
			break;
		}
		case M_ADD_CATEGORY:
		{
			BString name;
			if(msg->FindString("name",&name)==B_OK)
				AddCategory(name.String());
			break;
		}
		case M_DELETE_RECIPE:
		{
			BAlert *alert = new BAlert("Recibe","Really delete this recipe?","Yes","No",
										NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			
			alert->SetShortcut(0,'y');
			alert->SetShortcut(1,B_ESCAPE);
			if(alert->Go()==1)
				break;
			
			RecipeItem *item = GetCurrentRecipe();
			if(item)
			{
				int32 index = fResults->IndexOf(item);
				
				DeleteRecipe(item->fNumber, item->fCategory.String());
				fResults->RemoveItem(item);
				delete item;
				
				if(fResults->CountItems() > 0)
				{
					if(fResults->ItemAt(index))
						fResults->Select(index);
					else
						fResults->Select(index-1);
				}
			}
			break;
		}
		case M_EXPORT_RECIPE:
		{
			entry_ref ref;
			BString name;
			if(msg->FindRef("directory",&ref) != B_OK ||
				msg->FindString("name",&name) != B_OK)
				break;
			
			BFile file;
			BDirectory dir(&ref);
			dir.CreateFile(name.String(),&file);
			BTranslationUtils::WriteStyledEditFile(fRecipe,&file);
			break;
		}
		case M_PRINT_RECIPE:
		{
			if(GetCurrentRecipe())
				DoPrint();
			break;
		}
		case M_LOOKUP_RECIPE:
		{
			int32 number;
			BString cat,name;
			if(msg->FindInt32("number",&number) == B_OK && 
				msg->FindString("category",&cat) == B_OK &&
				msg->FindString("name",&name) == B_OK)
			{
				ClearResults();
				SetRecipe(number,cat.String());
				fResults->AddItem(new RecipeItem(name.String(),cat.String(),number));
				fResults->Select(0);
				fResults->MakeFocus(true);
			}
			break;
		}
		case M_FIND_RECIPE:
		{
			msg->FindString("name",&fSearchName);
			msg->FindString("category",&fSearchCategory);
			msg->FindString("ingredients",&fSearchIngredients);
			
			thread_id id = spawn_thread(SearchThread,"searchthread",B_NORMAL_PRIORITY,this);
			resume_thread(id);
			break;
		}
		case M_RECATEGORIZE_RECIPE:
		{
			int32 number;
			BString oldcat, newcat;
			msg->FindInt32("number",&number);
			msg->FindString("oldcategory",&oldcat);
			msg->FindString("newcategory",&newcat);
			
			if(number < 0)
			{
				int32 index = 0, selection;
				Window()->BeginViewTransaction();
				do
				{
					selection = fResults->CurrentSelection(index);
					index++;
					if(selection >=0)
					{
						RecipeItem *item = (RecipeItem*)fResults->ItemAt(selection);
						if(item)
						{
							ChangeCategory(item->fNumber, item->fCategory.String(),
											newcat.String());
						}
					}
					
				} while(selection >= 0);
				Window()->EndViewTransaction();
				ClearResults();
				fRecipe->SetText("");
				
				thread_id id = spawn_thread(SearchThread,"searchthread",B_NORMAL_PRIORITY,this);
				resume_thread(id);
			}
			else
			{
				int32 selection = fResults->CurrentSelection();
				RecipeItem *item = (RecipeItem*)fResults->ItemAt(selection);
				ChangeCategory(number, oldcat.String(), newcat.String());
				fResults->RemoveItem(item);
				delete item;
				fResults->Select(selection);
			}
			break;
		}
		case M_SET_RECIPE:
		{
			RecipeItem *item = GetCurrentRecipe();
			if(item)
				SetRecipe(item->fNumber, item->fCategory.String());
			break;
		}
		case M_ADD_BOOKMARK:
		{
			RecipeItem *item = GetCurrentRecipe();
			if(item)
				AddFavorite(item->fNumber, item->fCategory.String(), item->fName.String());
			break;
		}
		case M_DELETE_BOOKMARK:
		{
			RecipeItem *item = GetCurrentRecipe();
			if(item)
				RemoveFavorite(item->fNumber, item->fCategory.String(), item->fName.String());
			break;
		}
		case M_JUMP_TO_BOOKMARK:
		{
			int32 number;
			BString cat,name;
			if(msg->FindInt32("number",&number) == B_OK &&
				msg->FindString("category",&cat) == B_OK &&
				msg->FindString("name",&name) == B_OK)
			{
				if(RecipeExists(number,cat.String(),name.String()))
				{
					ClearResults();
					fResults->AddItem(new RecipeItem(name.String(),cat.String(), number));
					fResults->Select(0L);
				}
				else
				{
					BString errmsg;
					errmsg << "The recipe '" << name << "' could not be found. It must "
						"have been removed, so this bookmark will also be removed. "
						"Sorry about that.";
					BAlert *alert = new BAlert("Recibe",errmsg.String(),"OK");
					alert->Go();
					RemoveFavorite(number,cat.String(),name.String());
				}
			}
			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}

void ChefView::SetRecipe(const int32 &number, const char *category)
{
	if (number < 0 || !category)
		return;

	BString name, ingredients, directions;
	if(GetRecipe(number, category, name, ingredients, directions))
	{
		BString data;
		
		data << "Category: " << category;
		fCategoryLabel->SetText(data.String());
		
		fRecipe->SetText("");
		fRecipe->SetFontAndColor(&fHeaderFont);

		data = name << "\n\n";
		fRecipe->Insert(data.String());
		
		fRecipe->SetFontAndColor(be_plain_font);
		data = ingredients << "\n\n";
		data << directions;
		fRecipe->Insert(data.String());
	}
}

int32 ChefView::SearchThread(void *data)
{
	ChefView *view = (ChefView*)data;
	view->DoSearch();
	return 0;
}

void ChefView::DoSearch(void)
{
	if(fSearchCategory.Length() <= 0)
		return;
	
	SetSearching(true);

	BList recipes;
	SearchRecipes(fSearchCategory.String(), fSearchName.String(),
		fSearchIngredients.String(), 200, recipes);
	Window()->Lock();
	ClearResults();
	Window()->UpdateIfNeeded();
	Window()->Unlock();
	if(!recipes.IsEmpty())
	{
		for(int32 i = 0; i < recipes.CountItems(); i++)
		{
			RecipeRecord* recipe = static_cast<RecipeRecord*>(recipes.ItemAt(i));
			Window()->Lock();
			fResults->AddItem(new RecipeItem(recipe->name.String(),
				fSearchCategory.String(), recipe->number));
			if(fResults->CountItems()==1)
			{
				fResults->Select(0L);
				fResults->MakeFocus(true);
				
				// Relock to allow for a redraw after selecting an item. We need to do
				// this to help mitigate a race condition which crashes the app with
				// mistimed database access
				Window()->Unlock();
				Window()->Lock();
			}
			Window()->Unlock();
		}
	}
	FreeRecipes(recipes);
	SetSearching(false);
}

RecipeItem *ChefView::GetCurrentRecipe(void)
{
	int32 selection = fResults->CurrentSelection();
	if(selection < 0)
		return NULL;
	
	return (RecipeItem*)fResults->ItemAt(selection);
}

void ChefView::SetSearching(bool issearching)
{
	Window()->Lock();
	
	if(issearching)
	{
		fCategoryLabel->SetText("Searching");
		fFind->SetEnabled(false);
		fRecipeMenu->SetEnabled(false);
		fFavoritesMenu->SetEnabled(false);
	}
	else
	{
		fCategoryLabel->SetText(fSearchCategory.String());
		fFind->SetEnabled(true);
		fRecipeMenu->SetEnabled(true);
		fFavoritesMenu->SetEnabled(true);
	}
	
	Window()->Unlock();
}

bool ChefView::IsSearching(void)
{
	return !fFind->IsEnabled();
}

void ChefView::DoPrint()
{
	status_t result;

	if (fPrintSettings == NULL) {
		if (PageSetup() != B_OK)
			return;
	} 

	BPrintJob printJob("Recibe");
	printJob.SetSettings(new BMessage(*fPrintSettings));
	result = printJob.ConfigJob();
	if (result != B_OK)
		return;
	
	// information from printJob
	BRect printrect = printJob.PrintableRect();
	BRect pagerect = printJob.PaperRect();
	if(pagerect.Width() - printrect.Width() < 60)
		printrect.right = pagerect.right - 60;
	if(pagerect.Height() - printrect.Height() < 60)
		printrect.bottom = pagerect.bottom - 60;
	
	// Create the rendering view.
	BRect viewrect = printrect;
	viewrect.OffsetTo(0,0);
	BTextView *printview = new BTextView(viewrect,"printview",
										viewrect.InsetByCopy(10,10),0,0);
	printview->SetStylable(true);
	BWindow *win = new BWindow(printJob.PrintableRect(),"",B_TITLED_WINDOW,0);
	win->Hide();
	win->AddChild(printview);
	
	RecipeItem *item = GetCurrentRecipe();
	if (item == NULL)
		return;

	BString name, ingredients, directions;
	if(GetRecipe(item->fNumber, item->fCategory.String(), name, ingredients,
		directions))
	{
		printview->SetFontAndColor(&fHeaderFont);
		BString data = name << "\n\n";
		printview->Insert(data.String());
		
		printview->SetFontAndColor(be_plain_font);
		data = ingredients << "\n\n";
		data << directions;
		printview->Insert(data.String());
	}

	win->Show();
	
	int32 firstPage = printJob.FirstPage();
	int32 lastPage = printJob.LastPage();
   
	// lines eventually to be used to compute pages to print
	int32 firstLine = 0;
	int32 lastLine = printview->CountLines();

	// values to be computed
	int32 pagesInDocument = 1;
	int32 linesInDocument = printview->CountLines();

	int32 currentLine = 0;
	while (currentLine < linesInDocument)
	{
		float currentHeight = 0;
		while (currentHeight < printrect.Height() && currentLine < linesInDocument)
		{
			currentHeight += printview->LineHeight(currentLine);
			if (currentHeight < printrect.Height())
				currentLine++;
		}
		if (pagesInDocument == lastPage)
			lastLine = currentLine;

		if (currentHeight >= printrect.Height())
		{
			pagesInDocument++;
			if (pagesInDocument == firstPage)
				firstLine = currentLine;
		}
	}

	if (lastPage > pagesInDocument - 1)
	{
		lastPage = pagesInDocument - 1;
		lastLine = currentLine - 1;
	}

	printJob.BeginJob();
	
	// This is the page printing loop
	int32 printLine = firstLine;
	while (printLine < lastLine)
	{
		float currentHeight = 0;
		int32 firstLineOnPage = printLine;
		while (currentHeight < printrect.Height() && printLine < lastLine)
		{
			currentHeight += printview->LineHeight(printLine);
			if (currentHeight < printrect.Height())
				printLine++;
		}
		
		float top = 0;
		if (firstLineOnPage != 0)
			top = printview->TextHeight(0, firstLineOnPage - 1);
		
		float bottom = printview->TextHeight(0, printLine - 1);
		BRect textRect(0, top + 10, printrect.Width(), bottom + 10);
		printJob.DrawView(printview, textRect, BPoint(0,0));
		printJob.SpoolPage();
	}

	printJob.CommitJob();
	win->PostMessage(B_QUIT_REQUESTED);
}

status_t ChefView::PageSetup(void)
{
	BPrintJob printJob("Recibe");

	if (fPrintSettings != NULL)
		printJob.SetSettings(new BMessage(*fPrintSettings));

	status_t result = printJob.ConfigPage();
	if (result == B_OK) {
		delete fPrintSettings;
		fPrintSettings = printJob.Settings();
	}

	return result;
}

void ChefView::FrameResized(float w, float h)
{
	fRecipe->SetTextRect(fRecipe->Bounds().InsetByCopy(10,10));
}

void ChefView::AddFavorite(const int32 &number, const char *category, const char *name)
{
	if(!name || number < 0 || !category)
		return;
	
	if(fFavoritesMenu->CountItems() == 2)
		fFavoritesMenu->AddSeparatorItem();
	
	BMenuItem *submenuitem = fFavoritesMenu->FindItem(category);
	BMenu *submenu;
	if(!submenuitem)
	{
		submenu = new BMenu(category);
		InsertMenuSorted(fFavoritesMenu,submenu);
	}
	else
		submenu = submenuitem->Submenu();
	
	BMenuItem *favitem = new RecipeMenuItem(name,category,number,M_JUMP_TO_BOOKMARK);
	fFavoriteList.AddItem(favitem);
	InsertMenuItemSorted(submenu,favitem);
	submenu->SetTargetForItems(this);
}

void ChefView::RemoveFavorite(const int32 &number, const char *category, const char *name)
{
	BMenuItem *submenuitem = fFavoritesMenu->FindItem(category);
	if(!submenuitem)
		return;
	
	BMenu *submenu = submenuitem->Submenu();
	if(!submenu)
		return;
	
	BMenuItem *item = submenu->FindItem(name);
	if(!item)
		return;
	
	fFavoriteList.RemoveItem(item);
	submenu->RemoveItem(item);
	delete item;
	
	if(submenu->CountItems() == 0)
	{
		fFavoritesMenu->RemoveItem(submenuitem);
		delete submenuitem;
	}
	
	if(fFavoritesMenu->CountItems()==3)
	{
		item = fFavoritesMenu->RemoveItem(2);
		delete item;
	}
}

void ChefView::LoadFavorites(void)
{
	TextFile file("/boot/home/config/settings/Recibe_favorites", B_READ_ONLY);
	if(file.InitCheck() != B_OK)
		return;

	BString data = file.ReadLine();
	while(data.CountChars() > 0)
	{
		int32 pos = data.FindLast("\t");
		if(pos == B_ERROR)
			return;
		
		BString name = data.String() + pos + 1;
		data.Truncate(pos);
		
		pos = data.FindLast("\t");
		if(pos == B_ERROR)
			return;
		BString category = data.String() + pos + 1;
		data.Truncate(pos);
		
		int32 number = atol(data.String());
		
		AddFavorite(number,category.String(),name.String());
		
		data = file.ReadLine();
	}
}

void ChefView::SaveFavorites(void)
{
	BString data;
	for(int32 i = 0; i < fFavoriteList.CountItems(); i++)
	{
		RecipeMenuItem *item = (RecipeMenuItem*)fFavoriteList.ItemAt(i);
		data << item->fNumber << "\t" << item->fCategory << "\t" << item->fName << "\n";
	}
	
	BFile file("/boot/home/config/settings/Recibe_favorites",
				B_READ_WRITE | B_ERASE_FILE | B_CREATE_FILE);
	if(fFavoriteList.CountItems() > 0)
		file.Write(data.String(),data.Length());
}

void ChefView::ClearResults(void)
{
	Window()->Lock();
	Window()->BeginViewTransaction();
	fResults->DeselectAll();
	for(int32 i = 0; i < fResults->CountItems(); i++)
	{
		RecipeItem *item = (RecipeItem*)fResults->ItemAt(i);
		delete item;
	}
	fResults->MakeEmpty();
	Window()->EndViewTransaction();
	Window()->Unlock();
}

void InsertMenuItemSorted(BMenu *menu, BMenuItem *item)
{
	if(!menu || !item)
		return;
	
	for(int32 i = 0; i < menu->CountItems(); i++)
	{
		BMenuItem *current = menu->ItemAt(i);
		if(!current)
		{
			menu->AddItem(item,i);
			return;
		}
		
		if(strcmp(item->Label(),current->Label()) <= 0)
		{
			menu->AddItem(item, (i == 0) ? 0 : i - 1);
			return;
		}
	}
	menu->AddItem(item);
}

void InsertMenuSorted(BMenu *menu, BMenu *item)
{
	if(!menu || !item)
		return;
	
	for(int32 i = 0; i < menu->CountItems(); i++)
	{
		BMenuItem *current = menu->ItemAt(i);
		BMenu *submenu = current->Submenu();
		if(!submenu)
			continue;
		
		if(strcmp(item->Name(),submenu->Name()) <= 0)
		{
			menu->AddItem(item, i - 1);
			return;
		}
	}
	menu->AddItem(item);
}

