#ifndef CHEFWIN_H
#define CHEFWIN_H

#include <Window.h>

class ChefView;

class ChefWindow : public BWindow
{
public:
	ChefWindow(const BRect &rect);
	bool QuitRequested(void);
	void MessageReceived(BMessage *msg);

private:
	ChefView *fView;
};

#endif
