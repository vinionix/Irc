#include "Server.hpp"
#include "replies.hpp"

static bool validModeStart(const std::string& token)
{
	return !token.empty() && (token[0] == '+' || token[0] == '-');
}

void Server::handleMode(Client& client, const ParsedCommand& cmd)
{
	if (!client.isRegistered())
	{
		sendNumeric(client, ERR_NOTREGISTERED, "MODE", "You have not registered");
		return;
	}
	if (cmd.args.empty() || cmd.args[0].empty())
	{
		sendNumeric(client, ERR_NEEDMOREPARAMS, "MODE", "Not enough parameters");
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
		std::string modes = "+";
		std::string params;
		if (channel->isInviteOnly())
			modes += "i";
		if (channel->isTopicRestricted())
			modes += "t";
		if (!channel->getPassword().empty())
		{
			modes += "k";
			params += " " + channel->getPassword();
		}
		if (channel->getMaxUsers() != -1)
		{
			std::ostringstream limit;
			limit << channel->getMaxUsers();
			modes += "l";
			params += " " + limit.str();
		}
		sendNumeric(client, RPL_CHANNELMODEIS,
			channelName + " " + modes + params, "");
		return;
	}
	if (!channel->isOperator(client.getFd()))
	{
		sendNumeric(client, ERR_CHANOPRIVSNEEDED,
			channelName, "You're not channel operator");
		return;
	}

	size_t tokenIndex = 1;
	while (tokenIndex < cmd.args.size())
	{
		if (!validModeStart(cmd.args[tokenIndex]))
		{
			sendNumeric(client, ERR_UNKNOWNMODE, cmd.args[tokenIndex],
				"is unknown mode char to me");
			++tokenIndex;
			continue;
		}

		std::string modeString = cmd.args[tokenIndex++];
		char action = 0;
		for (size_t i = 0; i < modeString.size(); ++i)
		{
			char mode = modeString[i];
			if (mode == '+' || mode == '-')
			{
				action = mode;
				continue;
			}
			if (!action)
			{
				sendNumeric(client, ERR_UNKNOWNMODE,
					std::string(1, mode), "is unknown mode char to me");
				continue;
			}

			std::string argument;
			bool applied = false;
			if (mode == 'i')
			{
				channel->setInviteOnly(action == '+');
				applied = true;
			}
			else if (mode == 't')
			{
				channel->setTopicRestricted(action == '+');
				applied = true;
			}
			else if (mode == 'k')
			{
				if (action == '+')
				{
					if (tokenIndex >= cmd.args.size())
					{
						sendNumeric(client, ERR_NEEDMOREPARAMS,
							"MODE +k", "Not enough parameters");
						continue;
					}
					argument = cmd.args[tokenIndex++];
					channel->setPassword(argument);
				}
				else
					channel->setPassword("");
				applied = true;
			}
			else if (mode == 'o')
			{
				if (tokenIndex >= cmd.args.size())
				{
					sendNumeric(client, ERR_NEEDMOREPARAMS,
						"MODE " + std::string(1, action) + "o",
						"Not enough parameters");
					continue;
				}
				argument = cmd.args[tokenIndex++];
				Client* target = findClientByNick(argument);
				if (!target)
				{
					sendNumeric(client, ERR_NOSUCHNICK,
						argument, "No such nick/channel");
					continue;
				}
				if (!channel->hasClient(target->getFd()))
				{
					sendNumeric(client, ERR_USERNOTINCHANNEL,
						argument + " " + channelName,
						"They aren't on that channel");
					continue;
				}
				if (action == '+')
					channel->addOperator(target->getFd());
				else
					channel->removeOperator(target->getFd());
				applied = true;
			}
			else if (mode == 'l')
			{
				if (action == '+')
				{
					if (tokenIndex >= cmd.args.size())
					{
						sendNumeric(client, ERR_NEEDMOREPARAMS,
							"MODE +l", "Not enough parameters");
						continue;
					}
					argument = cmd.args[tokenIndex++];
					std::istringstream stream(argument);
					int limit;
					char extra;
					if (!(stream >> limit) || limit <= 0 || (stream >> extra))
					{
						sendNumeric(client, ERR_INVALIDMODEPARAM,
							channelName + " l " + argument,
							"Invalid mode parameter");
						continue;
					}
					channel->setMaxUsers(limit);
				}
				else
					channel->setMaxUsers(-1);
				applied = true;
			}
			else
			{
				sendNumeric(client, ERR_UNKNOWNMODE,
					std::string(1, mode), "is unknown mode char to me");
			}

			if (applied)
			{
				std::string change;
				change += action;
				change += mode;
				if (!argument.empty())
					change += " " + argument;
				broadcastToChannel(*channel,
					clientPrefix(client) + " MODE " + channelName + " " + change);
			}
		}
	}
}
