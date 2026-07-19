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

class Server{
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
		pollfd	createPollFd(int fd);
    public:
		Server(const std::string& port, const std::string& password);
		void startPoll(void);
		~Server();
};