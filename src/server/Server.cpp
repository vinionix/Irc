#include "Server.hpp"

static const CommandEntry g_commands[] =
{
    { "PASS", &Server::handlePass },
    { "NICK", &Server::handleNick },
    { "USER", &Server::handleUser },
    { "JOIN", &Server::handleJoin },
    { "PART", &Server::handlePart },
    { "MODE", &Server::handleMode },
    { "TOPIC", &Server::handleTopic },
    { "KICK", &Server::handleKick },
    { "INVITE", &Server::handleInvite },
    { "PRIVMSG", &Server::handlePrivmsg },
	{ "QUIT", &Server::handleQuit }
};

Server::Server(const std::string& port, const std::string& password) {
	validatePort(port);
	validatePassword(password);
	createAndConfigureSocket();
}

void Server::validatePort(const std::string& port) {
	unsigned int portNum;

	std::istringstream iss(port);

	for (size_t i = 0; i < port.length(); i++) {
		if (!std::isdigit(port[i])) {
			throw std::runtime_error("Invalid port number!");
		}
	}
	if (!(iss >> portNum)) {
		throw std::runtime_error("Invalid port number!");
	}
	_port = portNum;
}

void Server::validatePassword(const std::string& password) {
	if (!password.length()) {
		throw std::runtime_error("Invalid password!");
	}
	_password = password;
}

void Server::createAndConfigureSocket() {
	int opt = 1;

	if ((_serverFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		close(_serverFd);
		throw std::runtime_error("Failed to create socket!");
	}
	if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		close(_serverFd);
		throw std::runtime_error("Failed to set socket options!");
	}
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = INADDR_ANY;
	_address.sin_port = htons(_port);
	if (bind(_serverFd, (struct sockaddr *)&_address, sizeof(_address)) < 0) {
		close(_serverFd);
		throw std::runtime_error("Failed to bind socket!");
	}
	if (fcntl(_serverFd, F_SETFL, O_NONBLOCK) < 0) {
		close(_serverFd);
		throw std::runtime_error("Failed to set socket to non-blocking!");
	}
	if (listen(_serverFd, SOMAXCONN) < 0) {
		close(_serverFd);
		throw std::runtime_error("Failed to listen on socket!");
	}
}

pollfd	Server::createPollFd(int fd) {
	pollfd pfd;
				
	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	return (pfd);
}

ParsedCommand parseCommand(std::string& line) {
	std::istringstream	iss(line);
	ParsedCommand		cmd;
	std::string			token;

	if (!(iss >> cmd.command))
 	   return cmd;

	size_t trailingPos = line.find(" :");

	while (iss >> token) {
		if (token.at(0) == ':') {
			if (trailingPos != std::string::npos)
				token = line.substr(trailingPos + 2);
			cmd.args.push_back(token);
			break;
		}
		cmd.args.push_back(token);
	}

	// Temporary logs for commands and arguments
	std::cout << "command: " << cmd.command << std::endl;
	for (size_t i = 0; i < cmd.args.size(); i++) {
		std::cout << "arg: " << cmd.args[i] << std::endl;
	}
	std::cout << std::endl;

	return cmd;
}

void Server::processClientBuffer(Client& client) {
	size_t pos;

	while ((pos = client.inBuffer.find("\r\n")) != std::string::npos) {
	    std::string line = client.inBuffer.substr(0, pos);

		if (line.empty() || line.at(0) == ':')
			break; //throw specific error for trailing at command

	    client.inBuffer.erase(0, pos + 2);

		ParsedCommand cmd = parseCommand(line);
		if (cmd.command.empty())
			break;
	    dispatch(client, cmd);
	}
}

void Server::handlePass(Client& client, const ParsedCommand& cmd) {
	if (client.getPassOk()) {
		std::cout << "Client " << client.getFd() << " has already provided a password." << std::endl;
		return;
	}
	if (cmd.args.empty())
		return;
	if (cmd.args[0] == _password) {
		client.setPassOk(true);
		std::cout << "Client " << client.getFd() << " provided correct password." << std::endl;
	}
	else {
		client.setPassOk(false);
		std::cout << "Client " << client.getFd() << " provided incorrect password."	<< std::endl;
		disconnectClient(client);
		return;
	}	
}

void Server::handleNick(Client&, const ParsedCommand&) {

}

void Server::handleUser(Client& client, const ParsedCommand& cmd) {
	if (cmd.args.size() != 4) {
		std::cout << "Client " << client.getFd() << " sent invalid USER command." << std::endl;
		return;
	}
	if (cmd.args[0].empty() || cmd.args[1].empty() || cmd.args[2].empty() || cmd.args[3].empty()) {
		std::cout << "Client " << client.getFd() << " sent invalid USER command." << std::endl;
		return;
	}
	client.getUser().username = cmd.args[0];
	client.getUser().hostname = cmd.args[1];
	client.getUser().servername = cmd.args[2];
	client.getUser().realname = cmd.args[3];
}

void Server::handleJoin(Client&, const ParsedCommand&) {

}

void Server::handlePart(Client&, const ParsedCommand&) {

}

void Server::handleMode(Client&, const ParsedCommand&) {

}

void Server::handleTopic(Client&, const ParsedCommand&) {

}

void Server::handleKick(Client&, const ParsedCommand&) {

}

void Server::handleInvite(Client&, const ParsedCommand&) {

}

void Server::handlePrivmsg(Client&, const ParsedCommand&) {

}

void Server::handleQuit(Client&, const ParsedCommand&) {

}

void Server::dispatch(Client& client, const ParsedCommand& cmd)
{
    for (size_t i = 0; i < sizeof(g_commands) / sizeof(CommandEntry); ++i)
    {
        if (cmd.command == g_commands[i].name)
        {
            (this->*g_commands[i].handler)(client, cmd);
            return;
        }
    }

    // TODO: ERR_UNKNOWNCOMMAND
}

void Server::disconnectClient(Client& client) {
	if (_channels.size() > 0) {
		for (size_t i = 0; i < _channels.size(); i++) {
			if (_channels[i].hasClient(client.getFd())) {
				_channels[i].removeClient(client.getFd());
				//Do something to notify the other clients in the channel that this client has disconnected
			}
		}
	}
	for (size_t i = 0; i < _pollFds.size(); i++) {
		if (_pollFds[i].fd == client.getFd()) {
			_pollFds.erase(_pollFds.begin() + i);
			break;
		}
	}
	close(client.getFd());
	_clientFds.erase(client.getFd());
}

void Server::startPoll(void) {
	_pollFds.push_back(createPollFd(_serverFd));

	while(true) {
		poll(_pollFds.data(), _pollFds.size(), -1);

		for (size_t i = 0; i < _pollFds.size(); i++) {
			if (!(_pollFds[i].revents & POLLIN))
				continue;
			if (_pollFds[i].fd == _serverFd) {
				int clientFd;

				if ((clientFd = accept(_serverFd, NULL, NULL)) == -1)
					continue;

				Client c(clientFd);

				_clientFds.insert(std::make_pair(clientFd, c));

				_pollFds.push_back(createPollFd(clientFd));
			}
			else {
				char buffer[512];

				int bytes = recv(_pollFds[i].fd, buffer, sizeof(buffer), 0);

				if (bytes <= 0)
					disconnectClient(_clientFds[_pollFds[i].fd]);
				else {
					_clientFds[_pollFds[i].fd].inBuffer.append(buffer, bytes);
					processClientBuffer(_clientFds[_pollFds[i].fd]);
				}
			}
		}
	}
}

Server::~Server() {

}