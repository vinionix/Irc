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

class Server{
    private:
		unsigned int					_port;
		int								_serverFd;
		std::string						_password;
		bool							_isRunning;
		std::map<int, Client>			_clientFds;
		std::vector<Channel>			_channels;
		std::vector<pollfd>				_pollFds;

		void	validatePort(const std::string& port);
		void	validatePassword(const std::string& password);
    public:
		Server(const std::string& port, const std::string& password);
		~Server();
};