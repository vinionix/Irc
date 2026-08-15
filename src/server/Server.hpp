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
		void 	disconnectClient(Client&, const std::string& message = "Disconnected");
		void	processClientBuffer(Client&);
		bool	nickAlreadyInUse(const std::string&, int);
		bool	invalidNick(const std::string&) const;
		void	sendToClient(Client& client, const std::string& message);
		void	sendNumeric(Client& client, const std::string& code,
							const std::string& params, const std::string& msg);
		void	tryRegister(Client& client);
		Channel*	findChannel(const std::string& name);
		Client*	findClientByNick(const std::string& nick);

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