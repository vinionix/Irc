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
        std::string				_inputBuffer;
        std::string				_outputBuffer;
		std::set<std::string>	_channels;
    public:
        Client(/* args */);
        ~Client();
};