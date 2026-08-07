#include "Channel.hpp"

Channel::Channel(const std::string& name)
    : _name(name), _inviteOnly(false), _topicRestricted(false), _maxUsers(-1) {}

Channel::~Channel() {}

const std::string& Channel::getName() const {
    return _name;
}

const std::string& Channel::getTopic() const {
    return _topic;
}

void Channel::setTopic(const std::string& topic) {
    _topic = topic;
}

const std::string& Channel::getPassword() const {
    return _password;
}

void Channel::setPassword(const std::string& password) {
    _password = password;
}

bool Channel::isInviteOnly() const {
    return _inviteOnly;
}

void Channel::setInviteOnly(bool mode) {
    _inviteOnly = mode;
}

bool Channel::isTopicRestricted() const {
    return _topicRestricted;
}

void Channel::setTopicRestricted(bool mode) {
    _topicRestricted = mode;
}

int Channel::getMaxUsers() const {
    return _maxUsers;
}

void Channel::setMaxUsers(int limit) {
    _maxUsers = limit;
}

void Channel::addClient(int fd) {
    _users.insert(fd);
}

void Channel::removeClient(int fd) {
    _users.erase(fd);
    _operators.erase(fd);
    _invitedUsers.erase(fd);
}

bool Channel::hasClient(int fd) const {
    return _users.find(fd) != _users.end();
}

const std::set<int>& Channel::getUsers() const {
    return _users;
}

void Channel::addOperator(int fd) {
    _operators.insert(fd);
}

void Channel::removeOperator(int fd) {
    _operators.erase(fd);
}

bool Channel::isOperator(int fd) const {
    return _operators.find(fd) != _operators.end();
}

void Channel::addInvite(int fd) {
    _invitedUsers.insert(fd);
}

void Channel::removeInvite(int fd) {
    _invitedUsers.erase(fd);
}

bool Channel::isInvited(int fd) const {
    return _invitedUsers.find(fd) != _invitedUsers.end();
}
