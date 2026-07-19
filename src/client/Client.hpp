#pragma once
#include <iostream>
#include <set>

class Client{
    private:
        int						_clientFd;
        std::string				_nickname;
        std::string				_username;
        bool					_isRegistered;
        bool					_PassOk;
		std::set<std::string>	_channels;
    public:
        char					_inputBuffer[1024];
        char					_outputBuffer[1024];
        int getFd() const;
        Client();
        Client(int fd);
        ~Client();
};