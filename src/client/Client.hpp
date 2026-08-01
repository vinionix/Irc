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
        std::string getUsername() const;
        std::string setUsername(std::string username);

        std::string             inBuffer;
        std::string				outBuffer;
};