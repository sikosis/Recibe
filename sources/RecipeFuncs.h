#ifndef RECIPEFUNCS_H
#define RECIPEFUNCS_H

#include <List.h>
#include <String.h>
#include <SupportDefs.h>

struct RecipeRecord {
	RecipeRecord();

	int32 number;
	BString name;
	BString ingredients;
	BString directions;
};

// The recipe store is a directory. Each category is a subdirectory and each
// recipe is a flattened BMessage file with native Haiku attributes.
status_t OpenRecipeStore(const char* path);
void CloseRecipeStore();

status_t GetCategories(BList& categories);
void FreeCategories(BList& categories);
status_t GetRecipes(const char* category, int32 firstNumber, int32 lastNumber,
	bool sortByName, int32 limit, BList& recipes);
status_t SearchRecipes(const char* category, const char* nameWords,
	const char* ingredientWords, int32 limit, BList& recipes);
void FreeRecipes(BList& recipes);
int32 CountRecipes(const char* category);

status_t DeleteCategory(const char* name);
void DeleteRecipe(const int32& number, const char* category);
bool CategoryExists(const char* name);
void AddCategory(const char* name);
bool ChangeCategory(const int32& number, const char* oldcat, const char* newcat);
bool AddRecipe(const char* category, const char* name,
	const char* ingredients, const char* directions);
bool UpdateRecipe(const int32& number, const char* category,
	const char* name, const char* ingredients, const char* directions);
bool GetRecipe(const int32& number, const char* category,
	BString& name, BString& ingredients, BString& directions);
bool RecipeExists(const int32& number, const char* category, const char* name);

status_t TokenizeWords(const char* source, BList* stringarray);
void CapitalizeEachWord(BString& string);

BString EscapeIllegalCharacters(const char* instr);
BString DeescapeIllegalCharacters(const char* instr);
BString EscapeIllegalWords(const char* instr);
BString DeescapeIllegalWords(const char* instr);
bool HasIllegalWord(const char* instr);

#endif
