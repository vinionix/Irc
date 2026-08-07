#pragma once
#include <iostream>
#include <set>

class Client{
    private:
        int						_clientFd;
        std::string				_nickname;
        struct                  User
        {
            std::string username;
            std::string hostname;
            std::string servername;
            std::string realname;
        };
        User					_user;
        bool					_isRegistered;
        bool					_passOk;
		std::set<std::string>	_channels;
    public:
        Client();
        Client(int fd);
        ~Client();

        int getFd() const;
        User& getUser();
        void setPassOk(bool passOk);
        bool getPassOk() const;
        std::string getNick() const;
        void setNickname(const std::string&);

        void addChannel(const std::string& channelName);
        void removeChannel(const std::string& channelName);
        const std::set<std::string>& getChannels() const;

        std::string             inBuffer;
        std::string				outBuffer;
};