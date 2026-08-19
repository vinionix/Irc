#include "Server.hpp"
#include "replies.hpp"

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
	{ "QUIT", &Server::handleQuit },
	{ "PING", &Server::handlePing },
	{ "PONG", &Server::handlePong },
	{ "CAP", &Server::handleCap }
};

ParsedCommand parseCommand(std::string& line)
{
	ParsedCommand		cmd;
	std::istringstream	iss(line);
	std::string			token;

	if (!(iss >> cmd.command))
		return cmd;
	for (size_t i = 0; i < cmd.command.size(); ++i)
	{
		cmd.command[i] = static_cast<char>(
			std::toupper(static_cast<unsigned char>(cmd.command[i])));
	}
	while (iss >> token)
	{
		if (!token.empty() && token[0] == ':')
		{
			size_t pos = line.find(token);
			if (pos != std::string::npos)
				token = line.substr(pos + 1);
			cmd.args.push_back(token);
			break;
		}
		cmd.args.push_back(token);
	}
	return cmd;
}

Server::Server(const std::string& port, const std::string& password)
	: _serverFd(-1)
{
	validatePort(port);
	validatePassword(password);
	signal(SIGPIPE, SIG_IGN);
	createAndConfigureSocket();
}

Server::~Server()
{
	for (std::map<int, Client>::iterator it = _clientFds.begin();
		it != _clientFds.end(); ++it)
	{
		if (it->second.getFd() >= 0)
			close(it->second.getFd());
	}
	if (_serverFd >= 0)
		close(_serverFd);
}

void Server::validatePort(const std::string& port)
{
	unsigned int		portNum;
	std::istringstream	iss(port);

	if (port.empty())
		throw std::runtime_error("Invalid port number!");
	for (size_t i = 0; i < port.length(); ++i)
	{
		if (!std::isdigit(static_cast<unsigned char>(port[i])))
			throw std::runtime_error("Invalid port number!");
	}
	if (!(iss >> portNum) || portNum == 0 || portNum > 65535)
		throw std::runtime_error("Invalid port number!");
	_port = portNum;
}

void Server::validatePassword(const std::string& password)
{
	if (password.empty())
		throw std::runtime_error("Invalid password!");
	_password = password;
}

void Server::createAndConfigureSocket()
{
	int opt = 1;

	_serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_serverFd < 0)
		throw std::runtime_error("Failed to create socket!");
	if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR,
			&opt, sizeof(opt)) < 0)
	{
		close(_serverFd);
		_serverFd = -1;
		throw std::runtime_error("Failed to set socket options!");
	}
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = INADDR_ANY;
	_address.sin_port = htons(_port);
	if (bind(_serverFd, reinterpret_cast<struct sockaddr*>(&_address),
			sizeof(_address)) < 0)
	{
		close(_serverFd);
		_serverFd = -1;
		throw std::runtime_error("Failed to bind socket!");
	}
	if (fcntl(_serverFd, F_SETFL, O_NONBLOCK) < 0)
	{
		close(_serverFd);
		_serverFd = -1;
		throw std::runtime_error("Failed to set socket to non-blocking!");
	}
	if (listen(_serverFd, SOMAXCONN) < 0)
	{
		close(_serverFd);
		_serverFd = -1;
		throw std::runtime_error("Failed to listen on socket!");
	}
}

pollfd Server::createPollFd(int fd)
{
	pollfd pfd;

	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	return pfd;
}

void Server::processClientBuffer(Client& client)
{
	size_t pos;

	while (client.getFd() >= 0
		&& (pos = client.inBuffer.find("\r\n")) != std::string::npos)
	{
		std::string line = client.inBuffer.substr(0, pos);
		client.inBuffer.erase(0, pos + 2);
		if (line.empty() || line[0] == ':')
			continue;
		ParsedCommand cmd = parseCommand(line);
		if (!cmd.command.empty())
			dispatch(client, cmd);
	}
}

std::string Server::clientPrefix(const Client& client) const
{
	std::string username = client.getNick();

	if (!client.getUser().username.empty())
		username = client.getUser().username;
	return ":" + client.getNick() + "!" + username + "@localhost";
}

void Server::sendToClient(Client& client, const std::string& message)
{
	if (client.getFd() < 0)
		return;
	client.outBuffer += message + "\r\n";
	setWriteInterest(client.getFd(), true);
}

void Server::sendNumeric(Client& client, const std::string& code,
	const std::string& params, const std::string& msg)
{
	std::string nick = client.getNick().empty() ? "*" : client.getNick();
	std::string response = ":" SERVER_NAME " " + code + " " + nick;

	if (!params.empty())
		response += " " + params;
	if (!msg.empty())
		response += " :" + msg;
	sendToClient(client, response);
}

void Server::tryRegister(Client& client)
{
	if (client.isRegistered())
		return;
	if (!client.getPassOk() || client.getNick().empty()
		|| client.getUser().username.empty())
		return;

	client.setRegistered(true);
	sendNumeric(client, RPL_WELCOME, "",
		"Welcome to the Internet Relay Network "
		+ client.getNick() + "!" + client.getUser().username + "@localhost");
	sendNumeric(client, RPL_YOURHOST, "",
		"Your host is " SERVER_NAME ", running version 1.0");
	sendNumeric(client, RPL_CREATED, "", "This server was created today");
	sendNumeric(client, RPL_MYINFO, SERVER_NAME " 1.0 o itkol", "");
	sendNumeric(client, RPL_ISUPPORT,
		"CHANTYPES=#& PREFIX=(o)@ CHANMODES=,k,l,it",
		"are supported by this server");
	sendNumeric(client, RPL_ENDOFMOTD, "", "End of /MOTD command.");
}

Channel* Server::findChannel(const std::string& name)
{
	for (size_t i = 0; i < _channels.size(); ++i)
	{
		if (_channels[i].getName() == name)
			return &_channels[i];
	}
	return NULL;
}

Client* Server::findClientByNick(const std::string& nick)
{
	for (std::map<int, Client>::iterator it = _clientFds.begin();
		it != _clientFds.end(); ++it)
	{
		if (it->second.getFd() >= 0 && it->second.getNick() == nick)
			return &it->second;
	}
	return NULL;
}

void Server::setWriteInterest(int fd, bool enabled)
{
	for (size_t i = 0; i < _pollFds.size(); ++i)
	{
		if (_pollFds[i].fd != fd)
			continue;
		if (enabled)
			_pollFds[i].events |= POLLOUT;
		else
			_pollFds[i].events &= static_cast<short>(~POLLOUT);
		return;
	}
}

void Server::flushClientOutput(Client& client)
{
	if (client.outBuffer.empty())
	{
		setWriteInterest(client.getFd(), false);
		return;
	}

	ssize_t sent = send(client.getFd(), client.outBuffer.c_str(),
		client.outBuffer.size(), 0);
	if (sent > 0)
		client.outBuffer.erase(0, static_cast<size_t>(sent));
	else if (sent == 0)
	{
		disconnectClient(client);
		return;
	}
	if (client.getFd() >= 0 && client.outBuffer.empty())
		setWriteInterest(client.getFd(), false);
}

void Server::removeDisconnectedClients()
{
	for (std::map<int, Client>::iterator it = _clientFds.begin();
		it != _clientFds.end(); )
	{
		if (it->second.getFd() < 0)
		{
			std::map<int, Client>::iterator dead = it;
			++it;
			_clientFds.erase(dead);
		}
		else
			++it;
	}
}

void Server::removeEmptyChannels()
{
	for (std::vector<Channel>::iterator it = _channels.begin();
		it != _channels.end(); )
	{
		if (it->getUsers().empty())
			it = _channels.erase(it);
		else
			++it;
	}
}

void Server::broadcastToChannel(Channel& channel,
	const std::string& message, int skipFd)
{
	const std::set<int>& users = channel.getUsers();

	for (std::set<int>::const_iterator it = users.begin(); it != users.end(); ++it)
	{
		if (*it == skipFd)
			continue;
		std::map<int, Client>::iterator target = _clientFds.find(*it);
		if (target != _clientFds.end() && target->second.getFd() >= 0)
			sendToClient(target->second, message);
	}
}

void Server::disconnectClient(Client& client, const std::string& message)
{
	int fd = client.getFd();

	if (fd < 0)
		return;

	std::set<int> recipients;
	for (size_t i = 0; i < _channels.size(); ++i)
	{
		if (!_channels[i].hasClient(fd))
			continue;
		const std::set<int>& users = _channels[i].getUsers();
		for (std::set<int>::const_iterator it = users.begin();
			it != users.end(); ++it)
		{
			if (*it != fd)
				recipients.insert(*it);
		}
	}

	std::string quitMessage = clientPrefix(client) + " QUIT :" + message;
	for (std::set<int>::iterator it = recipients.begin();
		it != recipients.end(); ++it)
	{
		std::map<int, Client>::iterator target = _clientFds.find(*it);
		if (target != _clientFds.end() && target->second.getFd() >= 0)
			sendToClient(target->second, quitMessage);
	}
	for (size_t i = 0; i < _channels.size(); ++i)
		_channels[i].removeClient(fd);

	for (std::vector<pollfd>::iterator it = _pollFds.begin();
		it != _pollFds.end(); ++it)
	{
		if (it->fd == fd)
		{
			_pollFds.erase(it);
			break;
		}
	}
	close(fd);
	client.setFd(-1);
	removeEmptyChannels();
}

void Server::acceptClient()
{
	int clientFd = accept(_serverFd, NULL, NULL);

	if (clientFd < 0)
		return;
	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0)
	{
		close(clientFd);
		return;
	}
	_clientFds.insert(std::make_pair(clientFd, Client(clientFd)));
	_pollFds.push_back(createPollFd(clientFd));
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
	sendNumeric(client, ERR_UNKNOWNCOMMAND,
		cmd.command, "Unknown command");
}

void Server::startPoll()
{
	_pollFds.push_back(createPollFd(_serverFd));

	while (true)
	{
		removeDisconnectedClients();
		int result = poll(&_pollFds[0], _pollFds.size(), -1);
		if (result <= 0)
			continue;

		std::vector<pollfd> ready = _pollFds;
		for (size_t i = 0; i < ready.size(); ++i)
		{
			if (!ready[i].revents)
				continue;
			if (ready[i].fd == _serverFd)
			{
				if (ready[i].revents & POLLIN)
					acceptClient();
				continue;
			}

			std::map<int, Client>::iterator it = _clientFds.find(ready[i].fd);
			if (it == _clientFds.end() || it->second.getFd() < 0)
				continue;
			Client& client = it->second;

			if (ready[i].revents & (POLLERR | POLLNVAL))
			{
				disconnectClient(client);
				continue;
			}
			if (ready[i].revents & POLLIN)
			{
				char buffer[512];
				ssize_t bytes = recv(client.getFd(), buffer, sizeof(buffer), 0);
				if (bytes == 0)
				{
					disconnectClient(client);
					continue;
				}
				if (bytes > 0)
				{
					client.inBuffer.append(buffer, static_cast<size_t>(bytes));
					processClientBuffer(client);
				}
			}
			if (client.getFd() < 0)
				continue;
			if (ready[i].revents & POLLHUP)
			{
				disconnectClient(client);
				continue;
			}
			if (ready[i].revents & POLLOUT)
				flushClientOutput(client);
		}
	}
}
