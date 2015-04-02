
#include "menu.h"


///
/// Define Menu class methods
///

Menu::Menu() : shouldPopMenu(false)
{

}

bool Menu::shouldPopThisMenu()
{
	return shouldPopMenu;
}
///
/// Define the MainMenu class
/// 

MainMenu::MainMenu(std::shared_ptr<Texture> desertText, std::shared_ptr<Texture> forestTex, std::shared_ptr<Texture> helpTex, std::shared_ptr<Texture> optionsTex, 
	std::function<void()> desertEvent, std::function<void()> forestEvent, std::function<void()> helpEvent, std::function<void()> optionsEvent
	) 
	: Menu(), startDesertLevel(Button(desertText, 10, 10, 256, 32)), startForestLevel(Button(forestTex, 10, 50, 256, 32)), 
	helpButton(Button(helpTex, 10, 90, 256, 32)), optionsButton(Button(optionsTex, 10, 130, 256, 32)),
	desertEvent(desertEvent), forestEvent(forestEvent), helpEvent(helpEvent), optionsEvent(optionsEvent)

{
}

void MainMenu::draw(float deltaTime)
{
	startDesertLevel.draw();
	startForestLevel.draw();
	helpButton.draw();
	optionsButton.draw();
}

void MainMenu::update(MouseManager *manager, float deltaTime)
{
	if (startDesertLevel.inBounds(manager->x, manager->y) && manager->leftMouseButtonState == MouseManager::MOUSE_JUST_PRESSED)
	{
		desertEvent();
	}
	if (startForestLevel.inBounds(manager->x, manager->y) && manager->leftMouseButtonState == MouseManager::MOUSE_JUST_PRESSED)
	{
		forestEvent();
	}
	if (helpButton.inBounds(manager->x, manager->y) && manager->leftMouseButtonState == MouseManager::MOUSE_JUST_PRESSED)
	{
		helpEvent();
	}
	if (optionsButton.inBounds(manager->x, manager->y) && manager->leftMouseButtonState == MouseManager::MOUSE_JUST_PRESSED)
	{
		optionsEvent();
	}

}

///
/// Define the OptionsMenu class
/// 

OptionsMenu::OptionsMenu(std::shared_ptr<Texture> backTex) : Menu(), backButton(Button(backTex, 10, 10, 256, 32))
{
}

void OptionsMenu::draw(float deltaTime)
{
	backButton.draw();
}

void OptionsMenu::update(MouseManager *manager, float deltaTime)
{
	if (backButton.inBounds(manager->x, manager->y) && manager->leftMouseButtonState == MouseManager::MOUSE_JUST_PRESSED)
	{
		shouldPopMenu = true;
	}
}


///
/// define the HelpMenu class
///

HelpMenu::HelpMenu(std::shared_ptr<Texture> backTex, std::shared_ptr<Texture> guide) : Menu(), backButton(Button(backTex, 10, 10, 256, 32)), guide(guide)
{
}

void HelpMenu::draw(float deltaTime)
{
	backButton.draw();
	
	using namespace gl;
	float x = 30;
	float y = 50;
	float width = 512;
	float height = 512;
	glDisable(GL_BLEND);
	glAlphaFunc(GL_GREATER, 0.1f);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
	guide->bind();
	glColor4f(1, 1, 1, 1);
	glBegin(GL_QUADS);
		glVertex3d(x, y + height, 0);
		glTexCoord2f(1, 0);
		glVertex3d(x + width, y + height, 0);
		glTexCoord2f(1, 1);
		glVertex3d(x + width, y, 0);
		glTexCoord2f(0, 1);
		glVertex3d(x, y, 0);
		glTexCoord2f(0, 0);
	glEnd();
}

void HelpMenu::update(MouseManager *manager, float deltaTime)
{
	if (backButton.inBounds(manager->x, manager->y) && manager->leftMouseButtonState == MouseManager::MOUSE_JUST_PRESSED)
	{
		shouldPopMenu = true;
	}
}

///
/// Define the GameOverMenu class
/// 

GameOverMenu::GameOverMenu(std::shared_ptr<Texture> backTex) : Menu(), backButton(Button(backTex, 10, 10, 256, 32))
{
}

void GameOverMenu::draw(float deltaTime)
{
	backButton.draw();
}

void GameOverMenu::update(MouseManager *manager, float deltaTime)
{
	if (backButton.inBounds(manager->x, manager->y) && manager->leftMouseButtonState == MouseManager::MOUSE_JUST_PRESSED)
	{
		shouldPopMenu = true;
	}
}