#ifndef RECIPEFUNCS_H
#define RECIPEFUNCS_H

#include <String.h>
#include <List.h>
#include "CppSQLite3.h"

void OpenDatabase(const char *path);

status_t TokenizeWords(const char *source, BList *stringarray);
void CapitalizeEachWord(BString &string);

void DeleteRecipe(const int32 &number, const char *category);
bool CategoryExists(const char *name);
void AddCategory(const char *name);
bool ChangeCategory(const int32 &number, const char *oldcat, const char *newcat);
bool AddRecipe(const char *category, const char *name,
				const char *ingredients, const char *directions);
bool UpdateRecipe(const int32 &number, const char *category,
				const char *name, const char *ingredients, 
				const char *directions);
bool GetRecipe(const int32 &number, const char *category,
				BString &name, BString &ingredients, BString &directions);
bool RecipeExists(const int32 &number, const char *category,
				const char *name);

BString EscapeIllegalCharacters(const char *instr);
BString DeescapeIllegalCharacters(const char *instr);
BString EscapeIllegalWords(const char *instr);
BString DeescapeIllegalWords(const char *instr);
bool HasIllegalWord(const char *instr);

void DBCommand(const char *command, const char *functionname);
CppSQLite3Query DBQuery(const char *query, const char *functionname);

void LockDatabase(void);
void UnlockDatabase(void);
bool IsDatabaseLocked(void);

extern CppSQLite3DB gDatabase;

#endif
