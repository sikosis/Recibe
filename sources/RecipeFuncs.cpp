#include "RecipeFuncs.h"

#include <Locker.h>
#include <Entry.h>

#include <ctype.h>
#include <debugger.h>

#include "CppSQLite3.h"

CppSQLite3DB gDatabase;
static BLocker sDBLock;

void OpenDatabase(const char *path)
{
	// Open the database
	try
	{
		gDatabase.open(path);
	}
	catch(...)
	{
	}
	
	// Check for tables and create them, if necessary
	if (!gDatabase.tableExists("categories"))
	{
		try
		{
			gDatabase.execDML("create table categories (category varchar);");
		}
		catch(...)
		{
		}
		AddCategory("Misc");
	}
}

void DeleteRecipe(const int32 &number, const char *category)
{
	if(!category)
		return;
	
	BString command = "delete from ";
	command << EscapeIllegalCharacters(category) << " where number = " << number;
	DBCommand(command.String(),"ChangeCategory:remove from old");
}

void AddCategory(const char *name)
{
	if(!name)
		return;
	BString esccat = EscapeIllegalCharacters(name);
	
	BString command = "select category from categories where category = '";
	command << esccat << "';";
	
	// make sure the category doesn't already exist
	CppSQLite3Query query = DBQuery(command.String(),"AddCategory:check existing");
	if(!query.eof())
		return;
	
	command = "insert into categories values('";
	command << esccat << "');";
	DBCommand(command.String(),"AddCategory:add to category list");
	
	command = "create table ";
	command << esccat << " (number integer primary key, name varchar, "
			"ingredients varchar, directions varchar);";
	DBCommand(command.String(),"AddCategory:create category");
}

bool CategoryExists(const char *name)
{
	if(!name)
		return false;
	BString esccat = EscapeIllegalCharacters(name);
	
	BString command = "select category from categories where category = '";
	command << esccat << "';";
	
	// make sure the category doesn't already exist
	CppSQLite3Query query = DBQuery(command.String(),"CategoryExists:check existing");
	if(query.eof())
		return false;
	
	return true;
}

bool ChangeCategory(const int32 &number, const char *oldcat, const char *newcat)
{
	if(!newcat)
		return false;
	
	BString name, ingred, dir;
	if(!GetRecipe(number, oldcat, name, ingred, dir))
		return false;
	
	BString command = "insert into ";
	command << EscapeIllegalCharacters(newcat) << " values(NULL, '"
			<< EscapeIllegalCharacters(name.String())
			<< "', '" << EscapeIllegalCharacters(ingred.String())
			<< "', '" << EscapeIllegalCharacters(dir.String())
			<< "');";
	
	command = "delete from ";
	command << EscapeIllegalCharacters(oldcat) << " where number = " << number;
	DBCommand(command.String(),"ChangeCategory:remove from old");
	
	return true;
}

bool AddRecipe(const char *category, const char *name,
				const char *ingredients, const char *directions)
{
	if(!name || strlen(name) < 1 || !category || strlen(category) < 0)
		return false;
	
	BString esccat(EscapeIllegalCharacters(category));
	
	BString command = "insert into ";
	command << esccat << " values(NULL, '" << EscapeIllegalCharacters(name)
			<< "', '" << EscapeIllegalCharacters(ingredients)
			<< "', '" << EscapeIllegalCharacters(directions)
			<< "');";
	
	DBCommand(command.String(), "AddRecipe");
	
	return true;
}

bool UpdateRecipe(const int32 &number, const char *category,
				const char *name, const char *ingredients, const char *directions)
{
	if(!name || strlen(name) < 1 || number < 0 || !category)
		return false;
	
	BString esccat(EscapeIllegalCharacters(category));
	
	BString command = "update ";
	command << category << " set name = '" << EscapeIllegalCharacters(name)
			<< "' where number = " << number << "; "
			<< "update " << category << " set ingredients = '" << EscapeIllegalCharacters(ingredients)
			<< "' where number = " << number << "; "
			<< "update " << category << " set directions = '" << EscapeIllegalCharacters(directions)
			<< "' where number = " << number << "; " ;
	
	DBCommand(command.String(), "UpdateRecipe");
	
	return true;
}

bool GetRecipe(const int32 &number, const char *category,
				BString &name, BString &ingredients, BString &directions)
{
	if(!category || number < 0)
		return false;
		
	BString esccat = EscapeIllegalCharacters(category);
	BString command = "select * from ";
	command << esccat << " where number = " << number;
	
	CppSQLite3Query query = DBQuery(command.String(),"GetRecipe");
	if(!query.eof())
	{
		name = DeescapeIllegalCharacters(query.getStringField(1));
		ingredients = DeescapeIllegalCharacters(query.getStringField(2));
		directions = DeescapeIllegalCharacters(query.getStringField(3));
	}
	else
		return false;
	
	return true;
}

bool RecipeExists(const int32 &number, const char *category, const char *name)
{
	if(!category || number < 0 || !name)
		return false;
	
	if(!CategoryExists(category))
		return false;
		
	BString esccat = EscapeIllegalCharacters(category);
	BString command = "select name from ";
	command << esccat << " where number = " << number;
	
	CppSQLite3Query query = DBQuery(command.String(),"GetRecipe");
	if(!query.eof())
	{
		BString entryname = DeescapeIllegalCharacters(query.getStringField(1));
		return (entryname == name);
	}
	else
		return false;
	
	return true;
}

// Tokenizes a string by whitespace into a list of BStrings
// Returns B_ERROR if unable to parse the name or there are no words in the string
// Returns B_BAD_VALUE if given a NULL pointer or the list is not empty
status_t TokenizeWords(const char *source, BList *stringarray)
{
	if(!source || !stringarray || !stringarray->IsEmpty())
		return B_BAD_VALUE;
	
	// convert all tabs to spaces and eliminate consecutive spaces so that we can 
	// easily use strtok() 
	BString bstring(source);
	bstring.ReplaceAll('\t',' ');
	bstring.ReplaceAll("  "," ");

	char *workstr=new char[strlen(source)+1];
	strcpy(workstr,bstring.String());
	strtok(workstr," ");
	
	char *token=strtok(NULL," "),*lasttoken=workstr;
	
	if(!token)
	{
		delete workstr;
		stringarray->AddItem(new BString(bstring));
		return B_OK;
	}
	
	int32 length;
	BString *newword;
	
	while(token)
	{
		length=token-lasttoken;
		
		newword=new BString(lasttoken,length+1);
		lasttoken=token;
		stringarray->AddItem(newword);

		token=strtok(NULL," ");
	}
	
	length=strlen(lasttoken);
	newword=new BString(lasttoken,length+1);
	lasttoken=token;
	stringarray->AddItem(newword);
	
	
	delete [] workstr;

	return B_OK;
}

void CapitalizeEachWord(BString &string)
{
	if(string.CountChars()<1)
		return;
	
	char *index = string.LockBuffer(string.Length());
		
	int32 count = 0;
	int32 length = string.Length();
		
	do {
		// Find the first alphabetical character...
		for(; count < length; count++) {
			if (isalpha(index[count])) {
				// ...found! Convert it to uppercase.
				index[count] = toupper(index[count]);
				count++;
				break;
			}
		}
		// Now find the first non-alphabetical character,
		// and meanwhile, turn to lowercase all the alphabetical ones
		for(; count < length; count++) {
			if (isalpha(index[count]))
				index[count] = tolower(index[count]);
			else
				break;
		}
	} while (count < length);
				
	string.UnlockBuffer();
}


void DBCommand(const char *command, const char *functionname)
{
	if(!command)
		debugger("NULL database command in ChefView::DBCommand");
	if(!functionname)
		debugger("NULL function name in ChefView::DBCommand");
	
	sDBLock.Lock();
	try
	{
		gDatabase.execDML(command);
	}
	catch(CppSQLite3Exception &e)
	{
		BString msg("Database Exception in ");
		msg << functionname << ".\n\n" << e.errorMessage()
			<< "\n\nDatabase Exception Command: " << command << "\n";
		printf(msg.String());
		debugger(msg.String());
		sDBLock.Unlock();
	}
	sDBLock.Unlock();
}

void LockDatabase(void)
{
	sDBLock.Lock();
}

void UnlockDatabase(void)
{
	sDBLock.Unlock();
}

bool IsDatabaseLocked(void)
{
	return sDBLock.IsLocked();
}

CppSQLite3Query DBQuery(const char *query, const char *functionname)
{
	if(!query)
		debugger("NULL database command in ChefView::DBQuery");
	if(!functionname)
		debugger("NULL function name in ChefView::DBQuery");
	
	try
	{
		return gDatabase.execQuery(query);
	}
	catch(CppSQLite3Exception &e)
	{
		BString msg("Database Exception in ");
		msg << functionname << ".\n\n" << e.errorMessage()
			<< "\n\nDatabase Exception Query: " << query << "\n";
		printf(msg.String());
		debugger(msg.String());
		sDBLock.Unlock();
	}
	sDBLock.Unlock();
	
	// this will never be reached - just to shut up the compiler
	return CppSQLite3Query();
}



static const char *sIllegalCharacters[] =
	{ "!","@","#","$","%","^","&","*","(",")","-",
	  "+","=","{","}","[","]","\\","|",";",":","'",
	  "\"","<",">",",",".","/","?","`","~"," ","\n",
	  "\t", NULL
	};
static const char *sReplacementCharacters[] =
	{ "£21¥","£40¥","£23¥","£24¥","£25¥","£5e¥","£26¥","£2a¥","£28¥","£29¥","£2d¥",
	  "£2b¥","£3d¥","£7b¥","£7d¥","£5b¥","£5d¥","£5c¥","£7c¥","£3b¥","£3a¥","£27¥",
	  "£22¥","£3c¥","£3e¥","£2c¥","£2e¥","£2f¥","£3f¥","£60¥","£7e¥","£20¥","£0d¥",
	  "£09¥", NULL
	};
	
static const char *sIllegalWords[]=
	{"select","drop","create","delete","where","update","order","by",
		"and","or","in","not","between","aliases","join","union","alter",
		"functions","group","into","view", NULL };
static const char *sReplacementWords[]=
	{"¥select","¥drop","¥create","¥delete","¥where","¥update","¥order","¥by",
		"¥and","¥or","¥in","¥not","¥between","¥aliases","¥join","¥union","¥alter",
		"¥functions","¥group","¥into","¥view", NULL };

BString EscapeIllegalCharacters(const char *instr)
{
	// Because the £ symbol isn't allowed in a category but is a valid database character,
	// we'll use it as the escape character for illegal characters
	
	BString string(instr);
	if(string.CountChars()<1)
		return string;
	
	int32 i=0;
	while(sIllegalCharacters[i])
	{
		// Deescape just in case
		string.ReplaceAll(sReplacementCharacters[i],sIllegalCharacters[i]);
		
		string.ReplaceAll(sIllegalCharacters[i],sReplacementCharacters[i]);
		i++;
	}
	return string;
}

BString DeescapeIllegalCharacters(const char *instr)
{
	BString string(instr);
	if(string.CountChars()<1)
		return string;
	
	int32 i=0;
	while(sIllegalCharacters[i])
	{
		string.ReplaceAll(sReplacementCharacters[i],sIllegalCharacters[i]);
		i++;
	}
	return string;
}

BString EscapeIllegalWords(const char *instr)
{
	// Because the £ symbol isn't allowed in a category but is a valid database character,
	// we'll use it as the escape character for illegal characters
	
	BString string(instr);
	if(string.CountChars()<1)
		return string;
	
	string.RemoveAll("£");
	string.RemoveAll("¥");
	
	int32 i=0;
	while(sIllegalWords[i])
	{
		string.IReplaceAll(sIllegalWords[i],sReplacementWords[i]);
		i++;
	}
	return string;
}

BString DeescapeIllegalWords(const char *instr)
{
	BString string(instr);
	if(string.CountChars()<1)
		return string;
	
	int32 i=0;
	while(sIllegalWords[i])
	{
		string.IReplaceAll(sReplacementWords[i],sIllegalWords[i]);
		i++;
	}
	return string;
}

bool HasIllegalWord(const char *instr)
{
	int32 i=0;
	while(sIllegalWords[i])
	{
		BString string = sIllegalWords[i];
		if(string.IFindFirst(instr)==0)
			return true;
		i++;
	}
	return false;
}

