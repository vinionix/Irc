#include "Server.hpp"
#include "replies.hpp"

void Server::handlePass(Client& client, const ParsedCommand& cmd)
{
	if (client.isRegistered() || client.getPassOk())
	{
		sendNumeric(client, ERR_ALREADYREGISTRED, "", "You may not reregister");
		return;
	}
	if (cmd.args.empty())
	{
		sendNumeric(client, ERR_NEEDMOREPARAMS, "PASS", "Not enough parameters");
		return;
	}
	if (cmd.args[0] != _password)
	{
		client.setPassOk(false);
		sendNumeric(client, ERR_PASSWDMISMATCH, "", "Password incorrect");
		return;
	}
	client.setPassOk(true);
	tryRegister(client);
}

bool Server::nickAlreadyInUse(const std::string& nick, int clientFd)
{
	for (std::map<int, Client>::iterator it = _clientFds.begin();
		it != _clientFds.end(); ++it)
	{
		if (it->first != clientFd && it->second.getFd() >= 0
			&& it->second.getNick() == nick)
			return true;
	}
	return false;
}

bool Server::invalidNick(const std::string& nick) const
{
	const std::string special = "[]\\`_^{|}";

	if (nick.empty())
		return true;
	if (!std::isalpha(static_cast<unsigned char>(nick[0]))
		&& special.find(nick[0]) == std::string::npos)
		return true;
	for (size_t i = 1; i < nick.length(); ++i)
	{
		unsigned char c = static_cast<unsigned char>(nick[i]);
		if (!std::isalnum(c) && nick[i] != '-'
			&& special.find(nick[i]) == std::string::npos)
			return true;
	}
	return false;
}

void Server::handleNick(Client& client, const ParsedCommand& cmd)
{
	if (cmd.args.empty() || cmd.args[0].empty())
	{
		sendNumeric(client, ERR_NONICKNAMEGIVEN, "", "No nickname given");
		return;
	}
	if (invalidNick(cmd.args[0]))
	{
		sendNumeric(client, ERR_ERRONEUSNICKNAME,
			cmd.args[0], "Erroneous Nickname");
		return;
	}
	if (nickAlreadyInUse(cmd.args[0], client.getFd()))
	{
		sendNumeric(client, ERR_NICKNAMEINUSE,
			cmd.args[0], "Nickname is already in use");
		return;
	}

	std::string oldNick = client.getNick();
	if (client.isRegistered() && !oldNick.empty() && oldNick != cmd.args[0])
	{
		std::set<int> recipients;
		const std::set<std::string>& channels = client.getChannels();
		for (std::set<std::string>::const_iterator it = channels.begin();
			it != channels.end(); ++it)
		{
			Channel* channel = findChannel(*it);
			if (!channel)
				continue;
			const std::set<int>& users = channel->getUsers();
			recipients.insert(users.begin(), users.end());
		}
		std::string message = clientPrefix(client) + " NICK :" + cmd.args[0];
		for (std::set<int>::iterator it = recipients.begin();
			it != recipients.end(); ++it)
		{
			std::map<int, Client>::iterator target = _clientFds.find(*it);
			if (target != _clientFds.end() && target->second.getFd() >= 0)
				sendToClient(target->second, message);
		}
	}
	client.setNickname(cmd.args[0]);
	tryRegister(client);
}

void Server::handleUser(Client& client, const ParsedCommand& cmd)
{
	if (client.isRegistered() || !client.getUser().username.empty())
	{
		sendNumeric(client, ERR_ALREADYREGISTRED, "", "You may not reregister");
		return;
	}
	if (cmd.args.size() < 4)
	{
		sendNumeric(client, ERR_NEEDMOREPARAMS, "USER", "Not enough parameters");
		return;
	}
	client.getUser().username = cmd.args[0];
	client.getUser().hostname = cmd.args[1];
	client.getUser().servername = cmd.args[2];
	client.getUser().realname = cmd.args[3];
	tryRegister(client);
}

void Server::handleJoin(Client& client, const ParsedCommand& cmd)
{
	if (!client.isRegistered())
	{
		sendNumeric(client, ERR_NOTREGISTERED, "JOIN", "You have not registered");
		return;
	}
	if (cmd.args.empty() || cmd.args[0].empty())
	{
		sendNumeric(client, ERR_NEEDMOREPARAMS, "JOIN", "Not enough parameters");
		return;
	}

	std::string channelName = cmd.args[0];
	std::string key = cmd.args.size() > 1 ? cmd.args[1] : "";
	if (channelName[0] != '#' && channelName[0] != '&')
	{
		sendNumeric(client, ERR_NOSUCHCHANNEL, channelName, "No such channel");
		return;
	}

	Channel* channel = findChannel(channelName);
	if (!channel)
	{
		_channels.push_back(Channel(channelName));
		channel = &_channels.back();
		channel->addClient(client.getFd());
		channel->addOperator(client.getFd());
		if (!key.empty())
			channel->setPassword(key);
	}
	else
	{
		if (channel->hasClient(client.getFd()))
		{
			sendNumeric(client, ERR_USERONCHANNEL,
				client.getNick() + " " + channelName,
				"You are already on that channel");
			return;
		}
		if (channel->isInviteOnly() && !channel->isInvited(client.getFd()))
		{
			sendNumeric(client, ERR_INVITEONLYCHAN,
				channelName, "Cannot join channel (+i)");
			return;
		}
		if (!channel->getPassword().empty() && channel->getPassword() != key)
		{
			sendNumeric(client, ERR_BADCHANNELKEY,
				channelName, "Cannot join channel (+k)");
			return;
		}
		if (channel->getMaxUsers() != -1
			&& static_cast<int>(channel->getUsers().size())
				>= channel->getMaxUsers())
		{
			sendNumeric(client, ERR_CHANNELISFULL,
				channelName, "Cannot join channel (+l)");
			return;
		}
		channel->addClient(client.getFd());
		channel->removeInvite(client.getFd());
	}

	client.addChannel(channelName);
	broadcastToChannel(*channel, clientPrefix(client) + " JOIN " + channelName);

	if (!channel->getTopic().empty())
		sendNumeric(client, RPL_TOPIC, channelName, channel->getTopic());

	std::string names;
	const std::set<int>& users = channel->getUsers();
	for (std::set<int>::const_iterator it = users.begin(); it != users.end(); ++it)
	{
		std::map<int, Client>::iterator member = _clientFds.find(*it);
		if (member == _clientFds.end())
			continue;
		if (!names.empty())
			names += " ";
		if (channel->isOperator(*it))
			names += "@";
		names += member->second.getNick();
	}
	sendNumeric(client, RPL_NAMREPLY, "= " + channelName, names);
	sendNumeric(client, RPL_ENDOFNAMES,
		channelName, "End of /NAMES list.");
}

void Server::handlePart(Client& client, const ParsedCommand& cmd)
{
	if (!client.isRegistered())
	{
		sendNumeric(client, ERR_NOTREGISTERED, "PART", "You have not registered");
		return;
	}
	if (cmd.args.empty())
	{
		sendNumeric(client, ERR_NEEDMOREPARAMS, "PART", "Not enough parameters");
		return;
	}

	std::string reason = cmd.args.size() > 1 ? cmd.args[1] : "";
	std::istringstream channelList(cmd.args[0]);
	std::string channelName;
	while (std::getline(channelList, channelName, ','))
	{
		Channel* channel = findChannel(channelName);
		if (!channel)
		{
			sendNumeric(client, ERR_NOSUCHCHANNEL, channelName, "No such channel");
			continue;
		}
		if (!channel->hasClient(client.getFd()))
		{
			sendNumeric(client, ERR_NOTONCHANNEL,
				channelName, "You're not on that channel");
			continue;
		}
		std::string message = clientPrefix(client) + " PART " + channelName;
		if (!reason.empty())
			message += " :" + reason;
		broadcastToChannel(*channel, message);
		channel->removeClient(client.getFd());
		client.removeChannel(channelName);
	}
	removeEmptyChannels();
}

void Server::handleTopic(Client& client, const ParsedCommand& cmd)
{
	if (!client.isRegistered())
	{
		sendNumeric(client, ERR_NOTREGISTERED, "TOPIC", "You have not registered");
		return;
	}
	if (cmd.args.empty())
	{
		sendNumeric(client, ERR_NEEDMOREPARAMS, "TOPIC", "Not enough parameters");
		return;
	}

	std::string channelName = cmd.args[0];
	Channel* channel = findChannel(channelName);
	if (!channel)
	{
		sendNumeric(client, ERR_NOSUCHCHANNEL, channelName, "No such channel");
		return;
	}
	if (!channel->hasClient(client.getFd()))
	{
		sendNumeric(client, ERR_NOTONCHANNEL,
			channelName, "You're not on that channel");
		return;
	}
	if (cmd.args.size() == 1)
	{
		if (channel->getTopic().empty())
			sendNumeric(client, RPL_NOTOPIC, channelName, "No topic is set");
		else
			sendNumeric(client, RPL_TOPIC, channelName, channel->getTopic());
		return;
	}
	if (channel->isTopicRestricted() && !channel->isOperator(client.getFd()))
	{
		sendNumeric(client, ERR_CHANOPRIVSNEEDED,
			channelName, "You're not channel operator");
		return;
	}

	channel->setTopic(cmd.args[1]);
	broadcastToChannel(*channel, clientPrefix(client) + " TOPIC "
		+ channelName + " :" + cmd.args[1]);
}

void Server::handleKick(Client& client, const ParsedCommand& cmd)
{
	if (!client.isRegistered())
	{
		sendNumeric(client, ERR_NOTREGISTERED, "KICK", "You have not registered");
		return;
	}
	if (cmd.args.size() < 2)
	{
		sendNumeric(client, ERR_NEEDMOREPARAMS, "KICK", "Not enough parameters");
		return;
	}

	std::string channelName = cmd.args[0];
	std::string targetNick = cmd.args[1];
	std::string reason = cmd.args.size() > 2 ? cmd.args[2] : targetNick;
	Channel* channel = findChannel(channelName);
	if (!channel)
	{
		sendNumeric(client, ERR_NOSUCHCHANNEL, channelName, "No such channel");
		return;
	}
	if (!channel->hasClient(client.getFd()))
	{
		sendNumeric(client, ERR_NOTONCHANNEL,
			channelName, "You're not on that channel");
		return;
	}
	if (!channel->isOperator(client.getFd()))
	{
		sendNumeric(client, ERR_CHANOPRIVSNEEDED,
			channelName, "You're not channel operator");
		return;
	}

	Client* target = findClientByNick(targetNick);
	if (!target || !channel->hasClient(target->getFd()))
	{
		sendNumeric(client, ERR_USERNOTINCHANNEL,
			targetNick + " " + channelName, "They aren't on that channel");
		return;
	}

	std::string message = clientPrefix(client) + " KICK "
		+ channelName + " " + targetNick + " :" + reason;
	broadcastToChannel(*channel, message);
	target->removeChannel(channelName);
	channel->removeClient(target->getFd());
	removeEmptyChannels();
}

void Server::handleInvite(Client& client, const ParsedCommand& cmd)
{
	if (!client.isRegistered())
	{
		sendNumeric(client, ERR_NOTREGISTERED,
			"INVITE", "You have not registered");
		return;
	}
	if (cmd.args.size() < 2)
	{
		sendNumeric(client, ERR_NEEDMOREPARAMS,
			"INVITE", "Not enough parameters");
		return;
	}

	std::string targetNick = cmd.args[0];
	std::string channelName = cmd.args[1];
	Channel* channel = findChannel(channelName);
	if (!channel)
	{
		sendNumeric(client, ERR_NOSUCHCHANNEL, channelName, "No such channel");
		return;
	}
	if (!channel->hasClient(client.getFd()))
	{
		sendNumeric(client, ERR_NOTONCHANNEL,
			channelName, "You're not on that channel");
		return;
	}
	if (channel->isInviteOnly() && !channel->isOperator(client.getFd()))
	{
		sendNumeric(client, ERR_CHANOPRIVSNEEDED,
			channelName, "You're not channel operator");
		return;
	}

	Client* target = findClientByNick(targetNick);
	if (!target)
	{
		sendNumeric(client, ERR_NOSUCHNICK,
			targetNick, "No such nick/channel");
		return;
	}
	if (channel->hasClient(target->getFd()))
	{
		sendNumeric(client, ERR_USERONCHANNEL,
			targetNick + " " + channelName, "is already on channel");
		return;
	}

	channel->addInvite(target->getFd());
	sendNumeric(client, RPL_INVITING,
		targetNick + " " + channelName, "");
	sendToClient(*target, clientPrefix(client) + " INVITE "
		+ targetNick + " " + channelName);
}

void Server::handlePrivmsg(Client& client, const ParsedCommand& cmd)
{
	if (!client.isRegistered())
	{
		sendNumeric(client, ERR_NOTREGISTERED,
			"PRIVMSG", "You have not registered");
		return;
	}
	if (cmd.args.empty() || cmd.args[0].empty())
	{
		sendNumeric(client, ERR_NORECIPIENT,
			"", "No recipient given (PRIVMSG)");
		return;
	}
	if (cmd.args.size() < 2 || cmd.args[1].empty())
	{
		sendNumeric(client, ERR_NOTEXTTOSEND, "", "No text to send");
		return;
	}

	std::string targetName = cmd.args[0];
	std::string text = cmd.args[1];
	text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
	text.erase(std::remove(text.begin(), text.end(), '\n'), text.end());
	std::string message = clientPrefix(client) + " PRIVMSG "
		+ targetName + " :" + text;

	if (targetName[0] == '#' || targetName[0] == '&')
	{
		Channel* channel = findChannel(targetName);
		if (!channel)
		{
			sendNumeric(client, ERR_NOSUCHCHANNEL,
				targetName, "No such channel");
			return;
		}
		if (!channel->hasClient(client.getFd()))
		{
			sendNumeric(client, ERR_CANNOTSENDTOCHAN,
				targetName, "Cannot send to channel");
			return;
		}
		broadcastToChannel(*channel, message, client.getFd());
		return;
	}

	Client* target = findClientByNick(targetName);
	if (!target)
	{
		sendNumeric(client, ERR_NOSUCHNICK,
			targetName, "No such nick/channel");
		return;
	}
	sendToClient(*target, message);
}

void Server::handleQuit(Client& client, const ParsedCommand& cmd)
{
	std::string message = "Disconnected";

	if (!cmd.args.empty() && !cmd.args[0].empty())
		message = cmd.args[0];
	disconnectClient(client, message);
}

void Server::handlePing(Client& client, const ParsedCommand& cmd)
{
	if (cmd.args.empty() || cmd.args[0].empty())
	{
		sendNumeric(client, ERR_NEEDMOREPARAMS, "PING", "Not enough parameters");
		return;
	}
	sendToClient(client, ":" SERVER_NAME " PONG " SERVER_NAME " :" + cmd.args[0]);
}

void Server::handlePong(Client&, const ParsedCommand&)
{
}

void Server::handleCap(Client& client, const ParsedCommand& cmd)
{
	if (cmd.args.empty())
		return;

	std::string nick = client.getNick().empty() ? "*" : client.getNick();
	if (cmd.args[0] == "LS")
		sendToClient(client, ":" SERVER_NAME " CAP " + nick + " LS :");
	else if (cmd.args[0] == "REQ")
	{
		std::string request = cmd.args.size() > 1 ? cmd.args[1] : "";
		sendToClient(client, ":" SERVER_NAME " CAP " + nick
			+ " NAK :" + request);
	}
}
