#include "Client.hpp"

Client::Client(int fd) : _clientFd(fd), _isRegistered(false), _PassOk(false) {

}

Client::~Client() {

}