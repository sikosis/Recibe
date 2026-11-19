#include "RecipeEditor.h"

#include <Alert.h>
#include <Application.h>
#include <Font.h>
#include <ListView.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <ScrollView.h>
#include <StringItem.h>
#include <StringView.h>
#include <TextControl.h>
#include <string.h>

enum
{
	M_ADD_INGREDIENT_ROW = 'aigr',
	M_REMOVE_INGREDIENT_ROW = 'rigr',
	M_QUIT_APP = 'qapp'
};

static const char* kMeasurements[] = {
	"No measurement", "tsp", "tbsp", "cup", "cups", "mL", "L", "g", "kg",
	"oz", "lb", "pinch", "clove", "can", "packet", "piece", NULL
};


RecipeEditor::RecipeEditor(const BRect &frame, const BMessenger &msgr,
		const int32 &number, const char *category)
	:	BWindow(frame, "Add Recipe", B_FLOATING_WINDOW_LOOK,
			B_MODAL_SUBSET_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS),
		fMessenger(msgr),
		fNumber(number),
		fCategory(category != NULL ? category : "")
{
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	AddShortcut('Q', B_COMMAND_KEY, new BMessage(M_QUIT_APP));
	SetSizeLimits(640, 30000, 560, 30000);

	BView *back = new BView(Bounds(), "back", B_FOLLOW_ALL, B_WILL_DRAW);
	back->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(back);

	float right = Bounds().right - 10;
	float y = 10;

	fCategories = new BMenu("Categories");
	BList categories;
	GetCategories(categories);
	for (int32 i = 0; i < categories.CountItems(); i++) {
		BString* cat = static_cast<BString*>(categories.ItemAt(i));
		fCategories->AddItem(new BMenuItem(cat->String(), NULL));
	}
	FreeCategories(categories);
	fCategories->SetRadioMode(true);
	fCategories->SetLabelFromMarked(true);
	if (fCategories->CountItems() > 0)
		fCategories->ItemAt(0)->SetMarked(true);

	BMenuField *categoryField = new BMenuField(
		BRect(10, y, 310, y + 24), "category field", "Category",
		fCategories, B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE | B_NAVIGABLE_JUMP);
	back->AddChild(categoryField);
	categoryField->ResizeToPreferred();
	y = categoryField->Frame().bottom + 10;

	if (category != NULL) {
		BMenuItem *marked = fCategories->FindItem(category);
		if (marked != NULL)
			marked->SetMarked(true);
	}

	fNameBox = new AutoTextControl(BRect(10, y, right, y + 24), "namebox",
		"Name: ", NULL, NULL, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_PULSE_NEEDED | B_NAVIGABLE | B_NAVIGABLE_JUMP);
	fNameBox->SetEscapeCancel(true);
	fNameBox->SetDivider(be_plain_font->StringWidth("Name ") + 5);
	back->AddChild(fNameBox);
	fNameBox->ResizeTo(right - 10, fNameBox->Bounds().Height());
	y = fNameBox->Frame().bottom + 10;

	BStringView *label = new BStringView(BRect(10, y, 200, y + 20),
		"ingredients label", "Ingredients:");
	back->AddChild(label);
	label->ResizeToPreferred();
	y = label->Frame().bottom + 6;

	fQuantityBox = new BTextControl(BRect(10, y, 195, y + 26), "quantity",
		"Quantity / range:", "", NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP);
	fQuantityBox->SetDivider(be_plain_font->StringWidth("Quantity / range:") + 8);
	back->AddChild(fQuantityBox);

	fMeasurements = new BMenu("Measurement");
	fMeasurements->SetRadioMode(true);
	fMeasurements->SetLabelFromMarked(true);
	for (int32 i = 0; kMeasurements[i] != NULL; i++)
		fMeasurements->AddItem(new BMenuItem(kMeasurements[i], NULL));
	fMeasurements->ItemAt(0)->SetMarked(true);
	BMenuField *measurementField = new BMenuField(
		BRect(205, y, 330, y + 26), "measurement field", "",
		fMeasurements, B_FOLLOW_LEFT | B_FOLLOW_TOP);
	measurementField->SetDivider(0);
	back->AddChild(measurementField);

	fIngredients = new BMenu("Ingredient");
	fIngredients->SetRadioMode(true);
	fIngredients->SetLabelFromMarked(true);
	BList ingredientNames;
	GetIngredientNames(ingredientNames);
	for (int32 i = 0; i < ingredientNames.CountItems(); i++) {
		BString* ingredient = static_cast<BString*>(ingredientNames.ItemAt(i));
		fIngredients->AddItem(new BMenuItem(ingredient->String(), NULL));
	}
	FreeCategories(ingredientNames);
	if (fIngredients->CountItems() > 0)
		fIngredients->ItemAt(0)->SetMarked(true);
	BMenuField *ingredientField = new BMenuField(
		BRect(340, y, right - 90, y + 26), "ingredient field", "",
		fIngredients, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	ingredientField->SetDivider(0);
	back->AddChild(ingredientField);

	fAddIngredient = new BButton(BRect(right - 80, y, right, y + 26),
		"add ingredient", "Add", new BMessage(M_ADD_INGREDIENT_ROW),
		B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	back->AddChild(fAddIngredient);
	y += 36;

	BRect listRect(10, y, right - 100, y + 100);
	fIngredientList = new BListView(listRect, "ingredient rows",
		B_SINGLE_SELECTION_LIST, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	BScrollView *ingredientScroll = new BScrollView("ingredient rows scroll",
		fIngredientList, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, 0, false, true);
	back->AddChild(ingredientScroll);
	fRemoveIngredient = new BButton(BRect(right - 90, y, right, y + 26),
		"remove ingredient", "Remove", new BMessage(M_REMOVE_INGREDIENT_ROW),
		B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	back->AddChild(fRemoveIngredient);
	y = ingredientScroll->Frame().bottom + 8;

	label = new BStringView(BRect(10, y, 300, y + 20), "notes label",
		"Additional ingredients / freeform notes:");
	back->AddChild(label);
	label->ResizeToPreferred();
	y = label->Frame().bottom + 5;

	BRect notesRect(10, y, right - B_V_SCROLL_BAR_WIDTH, y + 80);
	BRect textRect = notesRect.OffsetToCopy(0, 0);
	textRect.InsetBy(8, 8);
	fIngredientNotes = new BTextView(notesRect, "ingredient notes", textRect,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_PULSE_NEEDED | B_NAVIGABLE | B_NAVIGABLE_JUMP);
	fIngredientNotes->SetDoesUndo(true);
	BScrollView *notesScroll = new BScrollView("ingredient notes scroll",
		fIngredientNotes, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, 0, false, true);
	back->AddChild(notesScroll);
	y = notesScroll->Frame().bottom + 8;

	label = new BStringView(BRect(10, y, 200, y + 20),
		"directions label", "Directions:");
	back->AddChild(label);
	label->ResizeToPreferred();
	y = label->Frame().bottom + 5;

	BRect directionsRect(10, y, right - B_V_SCROLL_BAR_WIDTH,
		Bounds().bottom - 55);
	textRect = directionsRect.OffsetToCopy(0, 0);
	textRect.InsetBy(8, 8);
	fDirectionsBox = new BTextView(directionsRect, "directions", textRect,
		B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED | B_NAVIGABLE
			| B_NAVIGABLE_JUMP);
	fDirectionsBox->SetDoesUndo(true);
	BScrollView *directionsScroll = new BScrollView("directions scroll",
		fDirectionsBox, B_FOLLOW_ALL, 0, false, true);
	back->AddChild(directionsScroll);

	fOK = new BButton(BRect(10, 10, 11, 11), "ok", "OK",
		new BMessage(M_ADD_RECIPE), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fOK->ResizeToPreferred();
	fOK->MoveTo(Bounds().right - fOK->Bounds().Width() - 10,
		Bounds().bottom - fOK->Bounds().Height() - 10);
	back->AddChild(fOK);

	BRect buttonRect = fOK->Frame();
	buttonRect.OffsetBy(-buttonRect.Width() - 10, 0);
	fCancel = new BButton(buttonRect, "cancel", "Cancel",
		new BMessage(B_QUIT_REQUESTED), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	back->AddChild(fCancel);

	if (number >= 0 && category != NULL) {
		SetTitle("Edit Recipe");
		BString name, ingredients, directions, ingredientNotes;
		if (GetRecipe(number, category, name, ingredients, directions,
				&fIngredientRecords, &ingredientNotes)) {
			fNameBox->SetText(name.String());
			fIngredientNotes->SetText(ingredientNotes.String());
			fDirectionsBox->SetText(directions.String());
			PopulateIngredientList();
			fOK->SetMessage(new BMessage(M_EDIT_RECIPE));
		}
	}

	AddShortcut(B_ENTER, B_COMMAND_KEY, new BMessage(fOK->Command()));
	fNameBox->MakeFocus(true);
}


void
RecipeEditor::PopulateIngredientList(void)
{
	for (size_t i = 0; i < fIngredientRecords.size(); i++) {
		BString line = FormatIngredient(fIngredientRecords[i]);
		fIngredientList->AddItem(new BStringItem(line.String()));
	}
}


void
RecipeEditor::AddIngredientRow(void)
{
	BMenuItem* ingredientItem = fIngredients->FindMarked();
	if (ingredientItem == NULL)
		return;

	IngredientRecord ingredient;
	ingredient.quantity = fQuantityBox->Text();
	ingredient.name = ingredientItem->Label();
	BMenuItem* measurementItem = fMeasurements->FindMarked();
	if (measurementItem != NULL
			&& strcmp(measurementItem->Label(), "No measurement") != 0) {
		ingredient.measurement = measurementItem->Label();
	}

	fIngredientRecords.push_back(ingredient);
	BString line = FormatIngredient(ingredient);
	fIngredientList->AddItem(new BStringItem(line.String()));
	fIngredientList->Select(fIngredientList->CountItems() - 1);
	fIngredientList->ScrollToSelection();
	fQuantityBox->SetText("");
	fQuantityBox->MakeFocus(true);
}


void
RecipeEditor::RemoveIngredientRow(void)
{
	int32 selection = fIngredientList->CurrentSelection();
	if (selection < 0 || selection >= (int32)fIngredientRecords.size())
		return;
	fIngredientRecords.erase(fIngredientRecords.begin() + selection);
	delete fIngredientList->RemoveItem(selection);
	if (selection >= fIngredientList->CountItems())
		selection = fIngredientList->CountItems() - 1;
	if (selection >= 0)
		fIngredientList->Select(selection);
}


void
RecipeEditor::SaveAndClose(bool editing)
{
	if (fNameBox->Text() == NULL || fNameBox->Text()[0] == '\0') {
		(new BAlert("Recibe", "Please enter a recipe name.", "OK"))->Go();
		return;
	}

	BMenuItem* categoryItem = fCategories->FindMarked();
	const char* destination = categoryItem != NULL
		? categoryItem->Label() : "Misc";
	BString combinedIngredients = BuildIngredientsText(fIngredientRecords,
		fIngredientNotes->Text());
	bool saved = false;
	if (editing && fCategory == destination) {
		saved = UpdateRecipe(fNumber, destination, fNameBox->Text(),
			combinedIngredients.String(), fDirectionsBox->Text(),
			&fIngredientRecords, fIngredientNotes->Text());
	} else if (editing) {
		saved = AddRecipe(destination, fNameBox->Text(),
			combinedIngredients.String(), fDirectionsBox->Text(),
			&fIngredientRecords, fIngredientNotes->Text());
		if (saved)
			DeleteRecipe(fNumber, fCategory.String());
	} else {
		saved = AddRecipe(destination, fNameBox->Text(),
			combinedIngredients.String(), fDirectionsBox->Text(),
			&fIngredientRecords, fIngredientNotes->Text());
	}

	if (!saved) {
		(new BAlert("Recibe", "The recipe could not be saved.", "OK"))->Go();
		return;
	}

	fMessenger.SendMessage(M_RECIPES_CHANGED);
	PostMessage(B_QUIT_REQUESTED);
}


void
RecipeEditor::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case M_QUIT_APP:
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		case M_ADD_INGREDIENT_ROW:
			AddIngredientRow();
			break;
		case M_REMOVE_INGREDIENT_ROW:
			RemoveIngredientRow();
			break;
		case M_EDIT_RECIPE:
			SaveAndClose(true);
			break;
		case M_ADD_RECIPE:
			SaveAndClose(false);
			break;
		default:
			BWindow::MessageReceived(message);
	}
}


void
RecipeEditor::FrameResized(float width, float height)
{
	fIngredientNotes->SetTextRect(
		fIngredientNotes->Bounds().InsetByCopy(8, 8));
	fDirectionsBox->SetTextRect(fDirectionsBox->Bounds().InsetByCopy(8, 8));
}
