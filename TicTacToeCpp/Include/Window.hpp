#pragma once


class Window
{
public:
	Window(const int windowWidth, const  int windowHeight, const std::string& windowName);
	~Window();
	sf::RenderWindow* RenderWindow = nullptr;
	void Rename(const std::string& newName); // make it flip between tic & tac and toe when sb wins
private:
	
};