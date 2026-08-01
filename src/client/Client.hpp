#pragma once
#include <iostream>
#include <set>

class Client{
    private:
        int						_clientFd;
        std::string				_nickname;
        std::string				_username;
        bool					_isRegistered;
        bool					_passOk;
		std::set<std::string>	_channels;
    public:
        Client();
        Client(int fd);
        ~Client();

        int getFd() const;
        void setPassOk(bool passOk);
        bool getPassOk() const;

        std::string             inBuffer;
        std::string				outBuffer;
};