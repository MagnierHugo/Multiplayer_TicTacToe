#include "GameServer.hpp"
#include "Plays.hpp"
#include "PacketSendTarget.hpp"
#include "Grid.hpp"

GameServer* GameServer::instance = nullptr;
bool GameServer::LogEveryPacket = false;

GameServer::GameServer(bool logEveryPacket_)
{
	instance = this;
	gameGrid = new Grid(true);

	startupPacket[0] = static_cast<int>(Plays::InvalidPlay);
	startupPacket[2] = static_cast<int>(false);

	ZeroMemory(gridState, 9); // for the sake of intellisense happy

	LogEveryPacket = logEveryPacket_;
}
GameServer::~GameServer()
{
	connectionThread->join();
	delete connectionThread;
}

#pragma optimize( "", off )
void GameServer::Run()
{
	connectionThread = new std::thread(
		[&]() {
			serverSocket.Run();
		}
	);

	while (serverSocket.GetConnectedClientsCount() < 1); // await one player

	std::cout << "Awaiting one more player to start the game..." << std::endl;

	while (serverSocket.GetConnectedClientsCount() > 0) // while at least one player is connected keep the server alive
	{
		if (serverSocket.GetConnectedClientsCount() < 2) { continue; } // await players to start the game

		serverSocket.Send(
			Grid::Serialize(gridState),
			SerializationHeaders::CatchupPacket,
			sizeof(char) * 9,
			PacketSendTarget::Broadcast
		);

		for (int i = 0; i < 2; i++) // loop over clients
		{
			startupPacket[1] = static_cast<int>(i == playerTurn); // wether that client has the turn
			serverSocket.Send(
				reinterpret_cast<char*>(startupPacket),
				SerializationHeaders::PlayResult, sizeof(int) * 3,
				static_cast<PacketSendTarget>(i)
			);
		}

		while (serverSocket.GetConnectedClientsCount() == 2); // await !players

		std::cout << "Awaiting another player to continue the game" << std::endl;
	}

	std::cout << "No more player connected. Shutting down the server" << std::endl;
}
#pragma optimize( "", on )

void GameServer::ParsePlay(const int play, int returnBuffer[3], const int clientNumber)
{
	/*
	if we branched into that method it means that
	returnBuffer is basically structured as such:
	{
	<int: the play itself>
	<int: wether u get the turn again>,
	<int: who played>
	}
	*/
	// lets set those for clarity and avoid magic values:
	static constexpr const int playItself = 0;
	static constexpr const int getTheTurnAgain = 1;
	static constexpr const int whoJustPlayed = 2;

	returnBuffer[whoJustPlayed] = clientNumber;
	bool validPlay = instance->CheckPlay(play, clientNumber);

	if (validPlay)
	{
		// the current client will not get the next turn
		returnBuffer[getTheTurnAgain] = static_cast<int>(false); // lowkey despise implicit casting

		bool gameEnded = false;
		if (gameEnded = Grid::CheckWin())
		{
			instance->score[clientNumber]++;
			GameServer::EndGame(static_cast<GameResult>(static_cast<int>(GameResult::PlayerZeroWon) + clientNumber), clientNumber);
			// bc both win result are in a row and <clientNumber> is 0 or 1 so the addition corrects the statement
			// im dogshit at explaining things but I swear it makes sense
			// don't worry I get it :D
		}
		else if (gameEnded = Grid::CheckDraw())
		{
			GameServer::EndGame(GameResult::Draw, clientNumber);
		}

		
		returnBuffer[playItself] = gameEnded ?
			static_cast<int>(Plays::InvalidPlay) : // to avoid changing client side grids
			play;
	}
	else
	{
		returnBuffer[playItself] = static_cast<int>(Plays::InvalidPlay); // u ve somehow managed not to understand how TicTacToe works
		returnBuffer[getTheTurnAgain] = static_cast<int>(true); // lowkey hate implicit casting
	}
}

void GameServer::EndGame(const GameResult gameResult, const int clientNumber)
{
	std::string msg;
	if (gameResult == GameResult::Draw)
	{
		msg = std::format("[Server] It s a draw\nScore: {} - {}", instance->score[0], instance->score[1]);
	}
	else
	{
		msg = std::format("[Server] Player {} won!\nScore: {} - {} ", clientNumber + 1, instance->score[0], instance->score[1]);
	}

	instance->serverSocket.Send(
		msg.c_str(),
		SerializationHeaders::ChatMessage,
		static_cast<int>(msg.size()) + 1,
		PacketSendTarget::Broadcast
	);

	Grid::Clear();

	instance->serverSocket.Send(
		Grid::Serialize(instance->gridState),
		SerializationHeaders::CatchupPacket,
		sizeof(char) * 9,
		PacketSendTarget::Broadcast
	);
}

bool GameServer::CheckPlay(const int play, const int clientNumber)
{
	if (clientNumber != playerTurn) { return false; }

	if (!Grid::IsSlotEmpty(play)) { return false; }

	playerTurn = playerTurn == 0 ? 1 : 0;
	Grid::Place(play, static_cast<bool>(clientNumber));
	
	return true;
}

void GameServer::RestoreChatMessages(const int clientIndex)
{
	PacketSendTarget target = static_cast<PacketSendTarget>(clientIndex);
	for (const auto& message : instance->chatLogs)
	{
		instance->serverSocket.Send(
			message.c_str(),
			SerializationHeaders::ChatMessage,
			static_cast<int>(message.size()) + 1,
			target
		);
	}
}
void GameServer::StoreChatMessage(const std::string& msg)
{
	instance->chatLogs.push_back(msg);
}
