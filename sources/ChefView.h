#ifndef CHEFVIEW_H
#define CHEFVIEW_H

#include <View.h>
#include <Button.h>
#include <StringView.h>
#include <String.h>
#include <TextView.h>
#include <ListView.h>
#include <Menu.h>
#include <OS.h>
#include <FilePanel.h>
#include <Font.h>

class RecipeItem;
class VSplitterView;

#define M_JUMP_TO_BOOKMARK 'jmbk'


class ChefView : public BView
{
public:
	ChefView(const BRect &rect);
	~ChefView(void);
	void MessageReceived(BMessage *msg);
	void AttachedToWindow(void);
	
	void SetRecipe(const int32 &number, const char *category);
	
	static int32 SearchThread(void *data);
	void DoSearch(void);
	void FrameResized(float w, float h);
	
private:
	void SetSearching(bool issearching);
	bool IsSearching(void);
	void DoPrint(void);
	status_t PageSetup(void);
	
	void ClearResults(void);
	
	void AddFavorite(const int32 &number, const char *category,
					const char *name);
	void RemoveFavorite(const int32 &number, const char *category,
					const char *name);
	
	void LoadFavorites(void);
	void SaveFavorites(void);

	RecipeItem *GetCurrentRecipe(void);
	
	VSplitterView	*fSplitter;
	BStringView	*fCategoryLabel;
	BTextView	*fRecipe;
	BListView	*fResults;
	
	BButton		*fFind;
	
	BMenu		*fFileMenu,
				*fRecipeMenu,
				*fFavoritesMenu,
				*fEmailMenu;
	
	BList		fFavoriteList;
	
	BFilePanel	*fSavePanel;
	
	BMessage	*fPrintSettings;
	
	BFont		fHeaderFont;
	
	BString		fSearchName,
				fSearchCategory,
				fSearchIngredients,
				fAllCategories;
};

class RecipeItem : public BStringItem
{
public:
	RecipeItem(const char *name, const char *category, const int32 &number);
	
	BString fName, fCategory;
	int32 fNumber;
};


void InsertMenuItemSorted(BMenu *menu, BMenuItem *item);
void InsertMenuSorted(BMenu *menu, BMenu *item);

#endif
