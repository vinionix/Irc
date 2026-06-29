#include "Server.hpp"

Server::Server(const std::string& port, const std::string& password) {
	validatePort(port);
	validatePassword(password);

	int opt = 1;
	if ((_serverFd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		close(_serverFd);
		throw std::runtime_error("Failed to create socket!");
	}
	if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
		close(_serverFd);
		throw std::runtime_error("Failed to set socket options!");
	}
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = INADDR_ANY;
	_address.sin_port = htons(_port);
	if (bind(_serverFd, (struct sockaddr *)&_address, sizeof(_address)) < 0){
		close(_serverFd);
		throw std::runtime_error("Failed to bind socket!");
	}
	if (fcntl(_serverFd, F_SETFL, O_NONBLOCK) < 0){
		close(_serverFd);
		throw std::runtime_error("Failed to set socket to non-blocking!");
	}
	if (listen(_serverFd, SOMAXCONN) < 0){
		close(_serverFd);
		throw std::runtime_error("Failed to listen on socket!");
	}
}

void Server::validatePort(const std::string& port) {
	unsigned int portNum;

	std::istringstream iss(port);

	for (size_t i = 0; i < port.length(); i++) {
		if (!std::isdigit(port[i])) {
			throw std::runtime_error("Invalid port number!");
		}
	}
	iss >> portNum;
	_port = portNum;
}

void Server::validatePassword(const std::string& password) {
	if (!password.length()) {
		throw std::runtime_error("Invalid password!");
	}
	_password = password;
}

Server::~Server() {

}