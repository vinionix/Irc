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

struct ParsedCommand {
	std::string					command;
	std::vector<std::string>	args;
};

class Server {
    private:
		unsigned int					_port;
		int								_serverFd;
		std::string						_password;
		bool							_isRunning;
		std::map<int, Client>			_clientFds;
		std::vector<Channel>			_channels;
		std::vector<pollfd>				_pollFds;
		struct sockaddr_in				_address;

		void	validatePort(const std::string& port);
		void	validatePassword(const std::string& password);
		void	createAndConfigureSocket(void);
		pollfd	createPollFd(int fd);
		void 	disconnectClient(Client&);
		void	processClientBuffer(Client&);

    public:
		Server(const std::string& port, const std::string& password);

		void startPoll(void);

		typedef void (Server::*CommandHandler)(Client&, const ParsedCommand&);

		void handlePass(Client&, const ParsedCommand&);
    	void handleNick(Client&, const ParsedCommand&);
    	void handleUser(Client&, const ParsedCommand&);
    	void handleJoin(Client&, const ParsedCommand&);
    	void handlePart(Client&, const ParsedCommand&);
    	void handleMode(Client&, const ParsedCommand&);
    	void handleTopic(Client&, const ParsedCommand&);
    	void handleKick(Client&, const ParsedCommand&);
    	void handleInvite(Client&, const ParsedCommand&);
    	void handlePrivmsg(Client&, const ParsedCommand&);
		void handleQuit(Client&, const ParsedCommand&);

    	void dispatch(Client&, const ParsedCommand&);

		~Server();
};

struct CommandEntry
{
    const char*     name;
    Server::CommandHandler handler;
};

ParsedCommand	parseCommand(std::string& line);