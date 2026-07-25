#pragma once
#include <iostream>
#include <map>
#include "../client/Client.hpp"
#include "../channel/Channel.hpp"
#include <vector>
#include <poll.h>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <algorithm>

class Server {
    private:
		struct sockaddr_in		_address;
		std::map<int, Client>	_clientFds;
		std::vector<Channel>	_channels;
		std::vector<pollfd>		_pollFds;
		std::string				_password;
		unsigned int			_port;
		int						_serverFd;
		bool					_isRunning;

		CommandStatus	execute(Client&, const ParsedCommand&);
		pollfd			createPollFd(int fd);
		void			validatePort(const std::string& port);
		void			validatePassword(const std::string& password);
		void			createAndConfigureSocket(void);
		void 			disconnectClient(Client&);
		void			processClientBuffer(Client&);


    public:
		Server(const std::string& port, const std::string& password);
		void startPoll(void);
		~Server();
};

enum CommandStatus
{
    CMD_OK,
    CMD_ERROR,
    CMD_DISCONNECT
};

struct ParsedCommand {
	std::string					command;
	std::vector<std::string>	args;
};

ParsedCommand	parseCommand(std::string& line);