#include "src/server/Server.hpp"

int main(int ac, char **av)
{
	if (ac != 3)
	{
		std::cerr << "Usage: " << av[0]
			<< " <port> <password>" << std::endl;
		return 1;
	}
	try
	{
		Server server(av[1], av[2]);
		server.startPoll();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
	return 0;
}
