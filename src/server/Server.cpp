#include "Server.hpp"

Server::Server(const std::string& port, const std::string& password) {
	validatePort(port);
	validatePassword(password);
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

Server::~Server() {

}