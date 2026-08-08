#include "Client.hpp"

int Client::getFd() const {
    return _clientFd;
}

void Client::setFd(int fd) {
    _clientFd = fd;
}

bool Client::getPassOk() const {
    return _passOk;
}

void Client::setPassOk(bool passOk) {
    _passOk = passOk;
}

bool Client::isRegistered() const {
    return _isRegistered;
}

void Client::setRegistered(bool registered) {
    _isRegistered = registered;
}

Client::User& Client::getUser() {
    return _user;
}

std::string Client::getNick() const {
    return _nickname;
}

void Client::setNickname(const std::string& nick) {
    _nickname = nick;
}

void Client::addChannel(const std::string& channelName) {
    _channels.insert(channelName);
}

void Client::removeChannel(const std::string& channelName) {
    _channels.erase(channelName);
}

const std::set<std::string>& Client::getChannels() const {
    return _channels;
}

Client::Client() : _clientFd(-1), _isRegistered(false), _passOk(false) {

}

Client::Client(int fd) : _clientFd(fd), _isRegistered(false), _passOk(false) {
}

Client::~Client() {

}