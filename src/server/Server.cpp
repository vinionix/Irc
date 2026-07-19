#include "Server.hpp"

Server::Server(const std::string& port, const std::string& password) {
	validatePort(port);
	validatePassword(password);
	createAndConfigureSocket();
}

void Server::validatePort(const std::string& port) {
	unsigned int portNum;

	std::istringstream iss(port);

	for (size_t i = 0; i < port.length(); i++) {
		if (!std::isdigit(port[i])) {
			throw std::runtime_error("Invalid port number!");
		}
	}
	if (!(iss >> portNum)) {
		throw std::runtime_error("Invalid port number!");
	}
	_port = portNum;
}

void Server::validatePassword(const std::string& password) {
	if (!password.length()) {
		throw std::runtime_error("Invalid password!");
	}
	_password = password;
}

void Server::createAndConfigureSocket() {
	int opt = 1;

	if ((_serverFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		close(_serverFd);
		throw std::runtime_error("Failed to create socket!");
	}
	if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		close(_serverFd);
		throw std::runtime_error("Failed to set socket options!");
	}
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = INADDR_ANY;
	_address.sin_port = htons(_port);
	if (bind(_serverFd, (struct sockaddr *)&_address, sizeof(_address)) < 0) {
		close(_serverFd);
		throw std::runtime_error("Failed to bind socket!");
	}
	if (fcntl(_serverFd, F_SETFL, O_NONBLOCK) < 0) {
		close(_serverFd);
		throw std::runtime_error("Failed to set socket to non-blocking!");
	}
	if (listen(_serverFd, SOMAXCONN) < 0) {
		close(_serverFd);
		throw std::runtime_error("Failed to listen on socket!");
	}
}

pollfd	Server::createPollFd(int fd) {
	pollfd pfd;
				
	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	return (pfd);
}

void Server::disconnectClient(Client& client) {
	if (_channels.size() > 0) {
		for (size_t j = 0; j < _channels.size(); j++) {
			if (_channels[j].hasClient(client.getFd())) {
				_channels[j].removeClient(client.getFd());
				//Do something to notify the other clients in the channel that this client has disconnected
			}
		}
	}
	for (size_t i = 0; i < _pollFds.size(); i++) {
		if (_pollFds[i].fd == client.getFd()) {
			_pollFds.erase(_pollFds.begin() + i);
			break;
		}
	}
	close(client.getFd());
	_clientFds.erase(client.getFd());
}

void Server::startPoll(void) {
	_pollFds.push_back(createPollFd(_serverFd));

	while(true) {
		poll(_pollFds.data(), _pollFds.size(), -1);

		for (size_t i = 0; i < _pollFds.size(); i++) {
			if (!(_pollFds[i].revents & POLLIN))
				continue;
			if (_pollFds[i].fd == _serverFd) {
				int clientFd;

				if ((clientFd = accept(_serverFd, NULL, NULL)) == -1)
					continue;

				Client c(clientFd);

				_clientFds.insert(std::make_pair(clientFd, c));

				_pollFds.push_back(createPollFd(clientFd));
			}
			else {
				char* bufferInput = _clientFds[_pollFds[i].fd]._inputBuffer;
				int size = sizeof(_clientFds[_pollFds[i].fd]._inputBuffer);
				int bytes = recv(_pollFds[i].fd, bufferInput, size, 0);

				if (bytes <= 0) {
					this->disconnectClient(_clientFds[_pollFds[i].fd]);
				}
				else {

				}
			}
		}
	}
}

Server::~Server() {

}