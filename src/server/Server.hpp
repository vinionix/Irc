#pragma once

#include <algorithm>
#include <cctype>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <poll.h>
#include <set>
#include <signal.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "../channel/Channel.hpp"
#include "../client/Client.hpp"

struct ParsedCommand
{
	std::string					command;
	std::vector<std::string>	args;
};

class Server
{
	private:
		unsigned int				_port;
		int						_serverFd;
		std::string					_password;
		std::map<int, Client>		_clientFds;
		std::vector<Channel>		_channels;
		std::vector<pollfd>			_pollFds;
		struct sockaddr_in			_address;

		void		validatePort(const std::string& port);
		void		validatePassword(const std::string& password);
		void		createAndConfigureSocket(void);
		pollfd		createPollFd(int fd);
		void		disconnectClient(Client& client,
						const std::string& message = "Disconnected");
		void		processClientBuffer(Client& client);
		bool		nickAlreadyInUse(const std::string& nick, int clientFd);
		bool		invalidNick(const std::string& nick) const;
		void		sendToClient(Client& client, const std::string& message);
		void		sendNumeric(Client& client, const std::string& code,
						const std::string& params, const std::string& msg);
		void		tryRegister(Client& client);
		Channel*	findChannel(const std::string& name);
		Client*		findClientByNick(const std::string& nick);
		void		setWriteInterest(int fd, bool enabled);
		void		flushClientOutput(Client& client);
		void		removeDisconnectedClients(void);
		void		removeEmptyChannels(void);
		void		broadcastToChannel(Channel& channel,
						const std::string& message, int skipFd = -1);
		std::string	clientPrefix(const Client& client) const;
		void		acceptClient(void);

	public:
		Server(const std::string& port, const std::string& password);
		~Server();

		void	startPoll(void);

		typedef void (Server::*CommandHandler)(Client&, const ParsedCommand&);

		void	handlePass(Client&, const ParsedCommand&);
		void	handleNick(Client&, const ParsedCommand&);
		void	handleUser(Client&, const ParsedCommand&);
		void	handleJoin(Client&, const ParsedCommand&);
		void	handlePart(Client&, const ParsedCommand&);
		void	handleMode(Client&, const ParsedCommand&);
		void	handleTopic(Client&, const ParsedCommand&);
		void	handleKick(Client&, const ParsedCommand&);
		void	handleInvite(Client&, const ParsedCommand&);
		void	handlePrivmsg(Client&, const ParsedCommand&);
		void	handleQuit(Client&, const ParsedCommand&);
		void	handlePing(Client&, const ParsedCommand&);
		void	handlePong(Client&, const ParsedCommand&);
		void	handleCap(Client&, const ParsedCommand&);

		void	dispatch(Client&, const ParsedCommand&);
};

struct CommandEntry
{
	const char*				name;
	Server::CommandHandler	handler;
};

ParsedCommand	parseCommand(std::string& line);
