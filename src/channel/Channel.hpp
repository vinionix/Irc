#pragma once

#include <iostream>
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
		Channel(const std::string& name);
		~Channel();

		const std::string&	getName() const;
		const std::string&	getTopic() const;
		void				setTopic(const std::string& topic);
		const std::string&	getPassword() const;
		void				setPassword(const std::string& password);
		bool				isInviteOnly() const;
		void				setInviteOnly(bool mode);
		bool				isTopicRestricted() const;
		void				setTopicRestricted(bool mode);
		int					getMaxUsers() const;
		void				setMaxUsers(int limit);

		void				addClient(int fd);
		void				removeClient(int fd);
		bool				hasClient(int fd) const;
		const std::set<int>&	getUsers() const;

		void				addOperator(int fd);
		void				removeOperator(int fd);
		bool				isOperator(int fd) const;

		void				addInvite(int fd);
		void				removeInvite(int fd);
		bool				isInvited(int fd) const;
};
