#include "GameServer.hpp"
#include "GameClient.hpp"


int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::cerr << "No startup args given" << std::endl;
		throw std::exception("Don t ever waste my time like this");
	}

	const bool isServer = strcmp(argv[1], "Server") == 0;

	std::string username;
	if (!isServer)
	{
		std::cout << "Enter your username: " << std::endl;
		std::cin >> username;
	}

	Game* game = (isServer ? (Game*)new GameServer() : (Game*)new GameClient(username));

	game->Run();

	delete game;
}
