#include "Client.hpp"

int Client::getFd() const {
    return _clientFd;
}

Client::Client() : _clientFd(-1), _isRegistered(false), _PassOk(false) {

}

Client::Client(int fd) : _clientFd(fd), _isRegistered(false), _PassOk(false) {

}

Client::~Client() {

}