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

		if (line.empty() || line.at(0) == ':') {
			client.inBuffer.erase(0, pos + 2);
			continue;
		}

	    client.inBuffer.erase(0, pos + 2);

		ParsedCommand cmd = parseCommand(line);
		if (cmd.command.empty())
			break;
	    dispatch(client, cmd);
	}
}

void Server::handlePass(Client& client, const ParsedCommand& cmd) {
	if (client.getPassOk()) {
		sendNumeric(client, ERR_ALREADYREGISTRED, "", "You may not reregister");
		return;
	}
	if (cmd.args.empty()) {
		sendNumeric(client, ERR_NEEDMOREPARAMS, "PASS", "Not enough parameters");
		return;
	}
	if (cmd.args[0] == _password) {
		client.setPassOk(true);
	}
	else {
		client.setPassOk(false);
		sendNumeric(client, ERR_PASSWDMISMATCH, "", "Password incorrect");
		disconnectClient(client);
		return;
	}
}

bool Server::nickAlreadyInUse(const std::string& nick, int clientFd) {
	for (size_t i = 0; i < _pollFds.size(); i++) {
		if (nick == _clientFds[_pollFds.at(i).fd].getNick()
			&& _pollFds.at(i).fd != clientFd) {
			return true;
		}
	}
	return false;
}

bool Server::invalidNick(const std::string& nick) const {
    if (nick.empty())
        return true;

    const std::string special = "[]\\`_^{|}";

    if (!std::isalpha(nick[0]) &&
        special.find(nick[0]) == std::string::npos)
        return true;

    for (size_t i = 1; i < nick.length(); ++i) {
        char c = nick[i];

        if (!std::isalnum(c) &&
            c != '-' &&
            special.find(c) == std::string::npos)
            return true;
    }

    return false;
}

void Server::handleNick(Client& client, const ParsedCommand& cmd) {
	if (cmd.args.empty() || cmd.args.at(0).empty()) {
		sendNumeric(client, ERR_NONICKNAMEGIVEN, "", "No nickname given");
		return;
	}
	if (invalidNick(cmd.args.at(0))) {
		sendNumeric(client, ERR_ERRONEUSNICKNAME, cmd.args.at(0), "Erroneous Nickname");
		return;
	}
	if (nickAlreadyInUse(cmd.args.at(0), client.getFd())) {
		sendNumeric(client, ERR_NICKNAMEINUSE, cmd.args.at(0), "Nickname is already in use");
		return;
	}

	client.setNickname(cmd.args.at(0));
}

void Server::handleUser(Client& client, const ParsedCommand& cmd) {
	if (!client.getUser().username.empty()) {
		sendNumeric(client, ERR_ALREADYREGISTRED, "", "You may not reregister");
		return;
	}
	if (cmd.args.size() != 4) {
		sendNumeric(client, ERR_NEEDMOREPARAMS, "USER", "Not enough parameters");
		return;
	}
	if (cmd.args[0].empty() || cmd.args[1].empty() || cmd.args[2].empty() || cmd.args[3].empty()) {
		sendNumeric(client, ERR_NEEDMOREPARAMS, "USER", "Not enough parameters");
		return;
	}
	client.getUser().username = cmd.args[0];
	client.getUser().hostname = cmd.args[1];
	client.getUser().servername = cmd.args[2];
	client.getUser().realname = cmd.args[3];
}

void Server::handleJoin(Client& client, const ParsedCommand& cmd) {
	if (client.getNick().empty()) {
		sendNumeric(client, ERR_NOTREGISTERED, "JOIN", "You have not registered");
		return;
	}
	if (cmd.args.empty() || cmd.args[0].empty()) {
		sendNumeric(client, ERR_NEEDMOREPARAMS, "JOIN", "Not enough parameters");
		return;
	}

	std::string channelName = cmd.args[0];
	std::string key = (cmd.args.size() > 1) ? cmd.args[1] : "";

	if (channelName[0] != '#' && channelName[0] != '&') {
		sendNumeric(client, ERR_NOSUCHCHANNEL, channelName, "No such channel");
		return;
	}

	Channel* channel = findChannel(channelName);
	bool created = false;

	if (!channel) {
		_channels.push_back(Channel(channelName));
		channel = &_channels.back();
		channel->addClient(client.getFd());
		channel->addOperator(client.getFd());
		created = true;
	}
	else {
		if (channel->isInviteOnly() && !channel->isInvited(client.getFd())) {
			sendNumeric(client, ERR_INVITEONLYCHAN, channelName, "Cannot join channel (+i)");
			return;
		}
		if (!channel->getPassword().empty() && channel->getPassword() != key) {
			sendNumeric(client, ERR_BADCHANNELKEY, channelName, "Cannot join channel (+k)");
			return;
		}
		if (channel->getMaxUsers() != -1 &&
			static_cast<int>(channel->getUsers().size()) >= channel->getMaxUsers()) {
			sendNumeric(client, ERR_CHANNELISFULL, channelName, "Cannot join channel (+l)");
			return;
		}
		if (channel->hasClient(client.getFd())) {
			sendNumeric(client, ERR_USERONCHANNEL, client.getNick() + " " + channelName,
				"You are already on that channel");
			return;
		}
		channel->addClient(client.getFd());
	}

	client.addChannel(channelName);

	sendToClient(client, ":" + client.getNick() + "!" + client.getUser().username
		+ " JOIN " + channelName);

	if (!channel->getTopic().empty()) {
		sendNumeric(client, RPL_TOPIC, channelName, channel->getTopic());
	}

	std::string names;
	const std::set<int>& users = channel->getUsers();
	for (std::set<int>::const_iterator it = users.begin(); it != users.end(); ++it) {
		if (it != users.begin())
			names += " ";
		if (channel->isOperator(*it)) {
			names += "@";
		}
		std::map<int, Client>::iterator clientIt = _clientFds.find(*it);
		if (clientIt != _clientFds.end()) {
			names += clientIt->second.getNick();
		}
	}

	sendNumeric(client, RPL_NAMREPLY, "= " + channelName, names);
	sendNumeric(client, RPL_ENDOFNAMES, channelName, "End of /NAMES list.");

	if (!created) {
		const std::set<int>& users = channel->getUsers();
		for (std::set<int>::const_iterator it = users.begin(); it != users.end(); ++it) {
			if (*it == client.getFd())
				continue;
			std::map<int, Client>::iterator clientIt = _clientFds.find(*it);
			if (clientIt != _clientFds.end()) {
				sendToClient(clientIt->second, ":" + client.getNick() + "!"
					+ client.getUser().username + " JOIN " + channelName);
			}
		}
	}
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

void Server::handleQuit(Client& c, const ParsedCommand& cmd) {
	static_cast<void>(cmd);
	disconnectClient(c);
}

void Server::sendToClient(Client& client, const std::string& message) {
    std::string msg = message + "\r\n";
    send(client.getFd(), msg.c_str(), msg.length(), 0);
}

void Server::sendNumeric(Client& client, const std::string& code,
                         const std::string& params, const std::string& msg) {
    std::string nick = client.getNick().empty() ? "*" : client.getNick();
    std::string response = ":" SERVER_NAME " " + code + " " + nick;
    if (!params.empty())
        response += " " + params;
    if (!msg.empty())
        response += " :" + msg;
    response += "\r\n";
    send(client.getFd(), response.c_str(), response.length(), 0);
}

Channel* Server::findChannel(const std::string& name) {
    for (size_t i = 0; i < _channels.size(); i++) {
        if (_channels[i].getName() == name)
            return &_channels[i];
    }
    return NULL;
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

    sendNumeric(client, ERR_UNKNOWNCOMMAND, cmd.command, "Unknown command");
}

void Server::disconnectClient(Client& client) {
	if (_channels.size() > 0) {
		for (size_t i = 0; i < _channels.size(); i++) {
			if (_channels[i].hasClient(client.getFd())) {
				const std::set<int>& users = _channels[i].getUsers();
				for (std::set<int>::const_iterator it = users.begin(); it != users.end(); ++it) {
					if (*it == client.getFd())
						continue;
					std::map<int, Client>::iterator clientIt = _clientFds.find(*it);
					if (clientIt != _clientFds.end()) {
						sendToClient(clientIt->second, ":" + client.getNick() + "!"
							+ client.getUser().username + " QUIT :Disconnected");
					}
				}
				_channels[i].removeClient(client.getFd());
			}
		}
	}
	for (size_t i = 0; i < _pollFds.size(); i++) {
		if (_pollFds.at(i).fd == client.getFd()) {
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
			if (!(_pollFds.at(i).revents & POLLIN))
				continue;
			if (_pollFds.at(i).fd == _serverFd) {
				int clientFd;

				if ((clientFd = accept(_serverFd, NULL, NULL)) == -1)
					continue;

				Client c(clientFd);

				_clientFds.insert(std::make_pair(clientFd, c));

				_pollFds.push_back(createPollFd(clientFd));
			}
			else {
				char buffer[512];

				int bytes = recv(_pollFds.at(i).fd, buffer, sizeof(buffer), 0);

				if (bytes <= 0)
					disconnectClient(_clientFds[_pollFds.at(i).fd]);
				else {
					_clientFds[_pollFds.at(i).fd].inBuffer.append(buffer, bytes);
					processClientBuffer(_clientFds[_pollFds.at(i).fd]);
				}
			}
		}
	}
}

Server::~Server() {

}