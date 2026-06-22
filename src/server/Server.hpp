#pragma once
#include <iostream>
#include <map>
#include "../client/Client.hpp"
#include "../channel/Channel.hpp"
#include <vector>
#include <poll.h>

class Server{
    private:
		int								_port;
		int								_serverFd;
		std::string						_password;
		bool							_isRunning;
		std::map<int, Client>			_clientFds;
		std::vector<Channel>			_channels;
		std::vector<pollfd>				_pollFds;
    public:
		Server(const int port, const std::string& password);
		~Server();
};