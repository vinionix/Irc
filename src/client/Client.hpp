#pragma once

#include <iostream>
#include <set>

class Client
{
	private:
		int						_clientFd;
		std::string				_nickname;
		struct User
		{
			std::string	username;
			std::string	hostname;
			std::string	servername;
			std::string	realname;
		};
		User					_user;
		bool					_isRegistered;
		bool					_passOk;
		std::set<std::string>	_channels;

	public:
		Client();
		Client(int fd);
		~Client();

		int							getFd() const;
		void						setFd(int fd);
		User&						getUser();
		const User&				getUser() const;
		void						setPassOk(bool passOk);
		bool						getPassOk() const;
		bool						isRegistered() const;
		void						setRegistered(bool registered);
		std::string					getNick() const;
		void						setNickname(const std::string& nick);
		void						addChannel(const std::string& channelName);
		void						removeChannel(const std::string& channelName);
		const std::set<std::string>&	getChannels() const;

		std::string					inBuffer;
		std::string					outBuffer;
};
