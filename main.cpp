#include "src/server/Server.hpp"

int main(int ac, char **av) {
    if (ac != 3) {
	    std::cerr << "Usage: " << av[0] << " <port> <password>" << std::endl;
        return (1);
    }
    try {
        Server s(static_cast<const std::string>(av[1]), static_cast<const std::string>(av[2]));
    }
    catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
        return 1;
    }
}