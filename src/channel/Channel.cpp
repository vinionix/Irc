#include "Channel.hpp"

bool Channel::hasClient(int fd) const {
    return _users.find(fd) != _users.end();
}

void Channel::removeClient(int fd) {
    _users.erase(fd);
}

Channel::~Channel() {

}