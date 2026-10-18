#include "RecipeFuncs.h"

#include <Autolock.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Locker.h>
#include <Message.h>
#include <TypeConstants.h>

#include <algorithm>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <vector>

static BLocker sStoreLock("recipe store");
static BString sStorePath;

static const char* kRecipeType = "application/x-vnd.whc-recibe-recipe";
static const char* kAttrName = "RECIBE:name";
static const char* kAttrCategory = "RECIBE:category";
static const char* kAttrNumber = "RECIBE:number";


RecipeRecord::RecipeRecord()
	:	number(-1)
{
}


static BString
EncodePathComponent(const char* value)
{
	static const char* digits = "0123456789ABCDEF";
	BString encoded;
	const unsigned char* byte = (const unsigned char*)(value ? value : "");
	for (; *byte != '\0'; byte++) {
		if ((*byte >= 'a' && *byte <= 'z') || (*byte >= 'A' && *byte <= 'Z')
			|| (*byte >= '0' && *byte <= '9') || *byte == '_' || *byte == '-') {
			char character = *byte;
			encoded.Append(&character, 1);
		} else {
			char escape[4] = {'%', digits[*byte >> 4], digits[*byte & 15], '\0'};
			encoded.Append(escape);
		}
	}
	return encoded;
}


static int
HexValue(char value)
{
	if (value >= '0' && value <= '9')
		return value - '0';
	if (value >= 'A' && value <= 'F')
		return value - 'A' + 10;
	if (value >= 'a' && value <= 'f')
		return value - 'a' + 10;
	return -1;
}


static BString
DecodePathComponent(const char* value)
{
	BString decoded;
	for (int32 i = 0; value != NULL && value[i] != '\0'; i++) {
		if (value[i] == '%' && value[i + 1] != '\0' && value[i + 2] != '\0') {
			int high = HexValue(value[i + 1]);
			int low = HexValue(value[i + 2]);
			if (high >= 0 && low >= 0) {
				char character = (char)((high << 4) | low);
				decoded.Append(&character, 1);
				i += 2;
				continue;
			}
		}
		decoded.Append(value + i, 1);
	}
	return decoded;
}


static BString
CategoryPath(const char* category)
{
	BString path(sStorePath);
	path << "/" << EncodePathComponent(category);
	return path;
}


static BString
RecipePath(const char* category, int32 number)
{
	BString path(CategoryPath(category));
	path << "/" << number << ".recipe";
	return path;
}


static status_t
LoadRecipeFile(const char* category, int32 number, RecipeRecord& recipe)
{
	BFile file(RecipePath(category, number).String(), B_READ_ONLY);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;

	BMessage archive;
	status = archive.Unflatten(&file);
	if (status != B_OK)
		return status;

	const char* value;
	if (archive.FindInt32("number", &recipe.number) != B_OK
		|| archive.FindString("name", &value) != B_OK) {
		return B_BAD_DATA;
	}
	recipe.name = value;
	recipe.ingredients = archive.FindString("ingredients", &value) == B_OK ? value : "";
	recipe.directions = archive.FindString("directions", &value) == B_OK ? value : "";
	return B_OK;
}


static status_t
WriteRecipeFile(const char* category, const RecipeRecord& recipe)
{
	BString path = RecipePath(category, recipe.number);
	BFile file(path.String(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;

	BMessage archive('Rcpe');
	archive.AddInt32("number", recipe.number);
	archive.AddString("name", recipe.name);
	archive.AddString("ingredients", recipe.ingredients);
	archive.AddString("directions", recipe.directions);
	status = archive.Flatten(&file);
	if (status != B_OK)
		return status;

	file.WriteAttr("BEOS:TYPE", B_MIME_STRING_TYPE, 0, kRecipeType,
		strlen(kRecipeType) + 1);
	file.WriteAttrString(kAttrName, &recipe.name);
	BString categoryName(category);
	file.WriteAttrString(kAttrCategory, &categoryName);
	file.WriteAttr(kAttrNumber, B_INT32_TYPE, 0, &recipe.number,
		sizeof(recipe.number));
	return file.Sync();
}


static int32
NextRecipeNumber(const char* category)
{
	BDirectory directory(CategoryPath(category).String());
	entry_ref ref;
	int32 highest = 0;
	while (directory.GetNextRef(&ref) == B_OK) {
		long number;
		if (sscanf(ref.name, "%ld.recipe", &number) == 1 && number > highest)
			highest = number;
	}
	return highest + 1;
}


static bool
RecipeNumberLess(const RecipeRecord* left, const RecipeRecord* right)
{
	return left->number < right->number;
}


static bool
RecipeNameLess(const RecipeRecord* left, const RecipeRecord* right)
{
	int compare = left->name.ICompare(right->name);
	return compare == 0 ? left->number < right->number : compare < 0;
}


static bool
CategoryNameLess(const BString* left, const BString* right)
{
	return left->ICompare(*right) < 0;
}


status_t
OpenRecipeStore(const char* path)
{
	if (path == NULL || path[0] == '\0')
		return B_BAD_VALUE;

	BAutolock lock(&sStoreLock);
	status_t status = create_directory(path, 0755);
	if (status != B_OK && status != B_FILE_EXISTS)
		return status;

	BDirectory directory(path);
	status = directory.InitCheck();
	if (status != B_OK)
		return status;

	sStorePath = path;
	if (!CategoryExists("Misc"))
		AddCategory("Misc");
	return B_OK;
}


void
CloseRecipeStore()
{
	BAutolock lock(&sStoreLock);
	sStorePath.Truncate(0);
}


status_t
GetCategories(BList& categories)
{
	if (!categories.IsEmpty())
		return B_BAD_VALUE;
	BAutolock lock(&sStoreLock);
	BDirectory directory(sStorePath.String());
	if (directory.InitCheck() != B_OK)
		return directory.InitCheck();

	std::vector<BString*> sorted;
	BEntry entry;
	while (directory.GetNextEntry(&entry) == B_OK) {
		if (!entry.IsDirectory())
			continue;
		char name[B_FILE_NAME_LENGTH];
		if (entry.GetName(name) == B_OK)
			sorted.push_back(new BString(DecodePathComponent(name)));
	}
	std::sort(sorted.begin(), sorted.end(), CategoryNameLess);
	for (size_t i = 0; i < sorted.size(); i++)
		categories.AddItem(sorted[i]);
	return B_OK;
}


void
FreeCategories(BList& categories)
{
	for (int32 i = categories.CountItems() - 1; i >= 0; i--)
		delete static_cast<BString*>(categories.RemoveItem(i));
}


status_t
GetRecipes(const char* category, int32 firstNumber, int32 lastNumber,
	bool sortByName, int32 limit, BList& recipes)
{
	if (category == NULL || !recipes.IsEmpty())
		return B_BAD_VALUE;
	BAutolock lock(&sStoreLock);
	BDirectory directory(CategoryPath(category).String());
	if (directory.InitCheck() != B_OK)
		return directory.InitCheck();

	std::vector<long> numbers;
	entry_ref ref;
	while (directory.GetNextRef(&ref) == B_OK) {
		long number;
		if (sscanf(ref.name, "%ld.recipe", &number) != 1
			|| number < firstNumber || (lastNumber >= 0 && number > lastNumber)) {
			continue;
		}
		numbers.push_back(number);
	}
	std::sort(numbers.begin(), numbers.end());

	std::vector<RecipeRecord*> sorted;
	for (size_t i = 0; i < numbers.size(); i++) {
		RecipeRecord* recipe = new RecipeRecord;
		if (LoadRecipeFile(category, numbers[i], *recipe) == B_OK)
			sorted.push_back(recipe);
		else
			delete recipe;
		if (!sortByName && limit > 0 && sorted.size() >= (size_t)limit)
			break;
	}
	std::sort(sorted.begin(), sorted.end(),
		sortByName ? RecipeNameLess : RecipeNumberLess);
	if (limit > 0 && sorted.size() > (size_t)limit) {
		for (size_t i = limit; i < sorted.size(); i++)
			delete sorted[i];
		sorted.resize(limit);
	}
	for (size_t i = 0; i < sorted.size(); i++)
		recipes.AddItem(sorted[i]);
	return B_OK;
}


static bool
ContainsAllWords(const BString& text, const char* words)
{
	if (words == NULL || words[0] == '\0')
		return true;
	BList tokens;
	if (TokenizeWords(words, &tokens) != B_OK)
		return true;
	bool matches = true;
	for (int32 i = 0; i < tokens.CountItems(); i++) {
		BString* token = static_cast<BString*>(tokens.ItemAt(i));
		if (text.IFindFirst(token->String()) < 0)
			matches = false;
		delete token;
	}
	return matches;
}


status_t
SearchRecipes(const char* category, const char* nameWords,
	const char* ingredientWords, int32 limit, BList& recipes)
{
	BList all;
	status_t status = GetRecipes(category, 0, -1, true, 0, all);
	if (status != B_OK)
		return status;
	for (int32 i = 0; i < all.CountItems(); i++) {
		RecipeRecord* recipe = static_cast<RecipeRecord*>(all.ItemAt(i));
		if (ContainsAllWords(recipe->name, nameWords)
			&& ContainsAllWords(recipe->ingredients, ingredientWords)
			&& (limit <= 0 || recipes.CountItems() < limit)) {
			recipes.AddItem(recipe);
		} else {
			delete recipe;
		}
	}
	all.MakeEmpty();
	return B_OK;
}


void
FreeRecipes(BList& recipes)
{
	for (int32 i = recipes.CountItems() - 1; i >= 0; i--)
		delete static_cast<RecipeRecord*>(recipes.RemoveItem(i));
}


int32
CountRecipes(const char* category)
{
	if (category == NULL)
		return 0;
	BAutolock lock(&sStoreLock);
	BDirectory directory(CategoryPath(category).String());
	if (directory.InitCheck() != B_OK)
		return 0;
	int32 count = directory.CountEntries();
	return count < 0 ? 0 : count;
}


void
DeleteRecipe(const int32& number, const char* category)
{
	if (category == NULL || number < 0)
		return;
	BAutolock lock(&sStoreLock);
	BEntry(RecipePath(category, number).String()).Remove();
}


void
AddCategory(const char* name)
{
	if (name == NULL || name[0] == '\0')
		return;
	BAutolock lock(&sStoreLock);
	BDirectory root(sStorePath.String());
	BDirectory category;
	root.CreateDirectory(EncodePathComponent(name).String(), &category);
}


bool
CategoryExists(const char* name)
{
	if (name == NULL || sStorePath.IsEmpty())
		return false;
	BAutolock lock(&sStoreLock);
	BEntry entry(CategoryPath(name).String());
	return entry.Exists() && entry.IsDirectory();
}


status_t
DeleteCategory(const char* name)
{
	if (name == NULL)
		return B_BAD_VALUE;
	BAutolock lock(&sStoreLock);
	BDirectory directory(CategoryPath(name).String());
	status_t status = directory.InitCheck();
	if (status != B_OK)
		return status;
	std::vector<BString> entries;
	entry_ref ref;
	while (directory.GetNextRef(&ref) == B_OK)
		entries.push_back(BString(ref.name));
	for (size_t i = 0; i < entries.size(); i++) {
		BEntry entry(&directory, entries[i].String());
		status = entry.Remove();
		if (status != B_OK)
			return status;
	}
	BEntry categoryEntry(CategoryPath(name).String());
	return categoryEntry.Remove();
}


bool
AddRecipe(const char* category, const char* name, const char* ingredients,
	const char* directions)
{
	if (category == NULL || name == NULL || name[0] == '\0')
		return false;
	BAutolock lock(&sStoreLock);
	if (!CategoryExists(category))
		AddCategory(category);
	RecipeRecord recipe;
	recipe.number = NextRecipeNumber(category);
	recipe.name = name;
	recipe.ingredients = ingredients ? ingredients : "";
	recipe.directions = directions ? directions : "";
	return WriteRecipeFile(category, recipe) == B_OK;
}


bool
UpdateRecipe(const int32& number, const char* category, const char* name,
	const char* ingredients, const char* directions)
{
	if (category == NULL || name == NULL || name[0] == '\0' || number < 0)
		return false;
	BAutolock lock(&sStoreLock);
	RecipeRecord recipe;
	recipe.number = number;
	recipe.name = name;
	recipe.ingredients = ingredients ? ingredients : "";
	recipe.directions = directions ? directions : "";
	return WriteRecipeFile(category, recipe) == B_OK;
}


bool
GetRecipe(const int32& number, const char* category, BString& name,
	BString& ingredients, BString& directions)
{
	if (category == NULL || number < 0)
		return false;
	BAutolock lock(&sStoreLock);
	RecipeRecord recipe;
	if (LoadRecipeFile(category, number, recipe) != B_OK)
		return false;
	name = recipe.name;
	ingredients = recipe.ingredients;
	directions = recipe.directions;
	return true;
}


bool
ChangeCategory(const int32& number, const char* oldcat, const char* newcat)
{
	if (oldcat == NULL || newcat == NULL || number < 0)
		return false;
	if (strcmp(oldcat, newcat) == 0)
		return true;
	BAutolock lock(&sStoreLock);
	RecipeRecord recipe;
	if (LoadRecipeFile(oldcat, number, recipe) != B_OK)
		return false;
	if (!CategoryExists(newcat))
		AddCategory(newcat);
	if (BEntry(RecipePath(newcat, number).String()).Exists())
		recipe.number = NextRecipeNumber(newcat);
	if (WriteRecipeFile(newcat, recipe) != B_OK)
		return false;
	return BEntry(RecipePath(oldcat, number).String()).Remove() == B_OK;
}


bool
RecipeExists(const int32& number, const char* category, const char* name)
{
	BString storedName, ingredients, directions;
	return name != NULL && GetRecipe(number, category, storedName, ingredients,
		directions) && storedName == name;
}


status_t
TokenizeWords(const char* source, BList* stringarray)
{
	if (source == NULL || stringarray == NULL || !stringarray->IsEmpty())
		return B_BAD_VALUE;
	const char* start = source;
	while (*start != '\0') {
		while (*start != '\0' && isspace((unsigned char)*start))
			start++;
		if (*start == '\0')
			break;
		const char* end = start;
		while (*end != '\0' && !isspace((unsigned char)*end))
			end++;
		stringarray->AddItem(new BString(start, end - start));
		start = end;
	}
	return stringarray->IsEmpty() ? B_ERROR : B_OK;
}


void
CapitalizeEachWord(BString& string)
{
	if (string.CountChars() < 1)
		return;
	char* index = string.LockBuffer(string.Length());
	int32 count = 0;
	int32 length = string.Length();
	do {
		for (; count < length; count++) {
			if (isalpha((unsigned char)index[count])) {
				index[count] = toupper((unsigned char)index[count]);
				count++;
				break;
			}
		}
		for (; count < length; count++) {
			if (isalpha((unsigned char)index[count]))
				index[count] = tolower((unsigned char)index[count]);
			else
				break;
		}
	} while (count < length);
	string.UnlockBuffer();
}


static const char* sIllegalCharacters[] = {
	"!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "-", "+", "=", "{", "}",
	"[", "]", "\\", "|", ";", ":", "'", "\"", "<", ">", ",", ".", "/", "?", "`",
	"~", " ", "\n", "\t", NULL
};
static const char* sReplacementCharacters[] = {
	"£21¥", "£40¥", "£23¥", "£24¥", "£25¥", "£5e¥", "£26¥", "£2a¥", "£28¥",
	"£29¥", "£2d¥", "£2b¥", "£3d¥", "£7b¥", "£7d¥", "£5b¥", "£5d¥", "£5c¥",
	"£7c¥", "£3b¥", "£3a¥", "£27¥", "£22¥", "£3c¥", "£3e¥", "£2c¥", "£2e¥",
	"£2f¥", "£3f¥", "£60¥", "£7e¥", "£20¥", "£0d¥", "£09¥", NULL
};
static const char* sIllegalWords[] = {
	"select", "drop", "create", "delete", "where", "update", "order", "by", "and", "or",
	"in", "not", "between", "aliases", "join", "union", "alter", "functions", "group",
	"into", "view", NULL
};
static const char* sReplacementWords[] = {
	"¥select", "¥drop", "¥create", "¥delete", "¥where", "¥update", "¥order", "¥by",
	"¥and", "¥or", "¥in", "¥not", "¥between", "¥aliases", "¥join", "¥union", "¥alter",
	"¥functions", "¥group", "¥into", "¥view", NULL
};


BString
EscapeIllegalCharacters(const char* instr)
{
	BString string(instr ? instr : "");
	for (int32 i = 0; sIllegalCharacters[i] != NULL; i++) {
		string.ReplaceAll(sReplacementCharacters[i], sIllegalCharacters[i]);
		string.ReplaceAll(sIllegalCharacters[i], sReplacementCharacters[i]);
	}
	return string;
}


BString
DeescapeIllegalCharacters(const char* instr)
{
	BString string(instr ? instr : "");
	for (int32 i = 0; sIllegalCharacters[i] != NULL; i++)
		string.ReplaceAll(sReplacementCharacters[i], sIllegalCharacters[i]);
	return string;
}


BString
EscapeIllegalWords(const char* instr)
{
	BString string(instr ? instr : "");
	string.RemoveAll("£");
	string.RemoveAll("¥");
	for (int32 i = 0; sIllegalWords[i] != NULL; i++)
		string.IReplaceAll(sIllegalWords[i], sReplacementWords[i]);
	return string;
}


BString
DeescapeIllegalWords(const char* instr)
{
	BString string(instr ? instr : "");
	for (int32 i = 0; sIllegalWords[i] != NULL; i++)
		string.ReplaceAll(sReplacementWords[i], sIllegalWords[i]);
	return string;
}


bool
HasIllegalWord(const char* instr)
{
	BString string(instr ? instr : "");
	for (int32 i = 0; sIllegalWords[i] != NULL; i++) {
		if (string.IFindFirst(sIllegalWords[i]) >= 0)
			return true;
	}
	return false;
}
