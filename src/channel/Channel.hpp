#pragma once

#include <iostream>
#include <map>
#include "../client/Client.hpp"
#include <set>

class Channel
{
    private:
        std::string		_name;
        std::string		_topic;
        std::string		_password;
        bool			_inviteOnly;
        bool			_topicRestricted;
        int				_maxUsers;
        std::set<int>	_users;
        std::set<int>	_operators;
		std::set<int>	_invitedUsers;

    public:
        bool hasClient(int fd) const;
        void removeClient(int fd);
        Channel(/* args */);
        ~Channel();
};
