#include <Application.h>
#include "ChefWindow.h"

class App : public BApplication
{
public:
	App(void);
};

App::App(void)
 :	BApplication("application/x-vnd.whc-Recibe")
{
	ChefWindow *win = new ChefWindow(BRect(100,100,600,400));
	win->Show();
}

int main(void)
{
	App app;
	app.Run();
	return 0;
}