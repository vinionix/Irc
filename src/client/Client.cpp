#include "Client.hpp"

int Client::getFd() const {
    return _clientFd;
}

bool Client::getPassOk() const {
    return _passOk;
}

void Client::setPassOk(bool passOk) {
    _passOk = passOk;
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

Client::Client() : _clientFd(-1), _isRegistered(false), _passOk(false) {

}

Client::Client(int fd) : _clientFd(fd), _isRegistered(false), _passOk(false) {
}

Client::~Client() {

}