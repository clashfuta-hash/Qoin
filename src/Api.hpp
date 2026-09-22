#pragma once
#include "utility.hpp"
#include "BSocket.hpp"
#include "Socketpp.hpp"
#include <nlohmann/json.hpp>

class Api{
public:
	// Constructor
	Api();
	// Destructor
	~Api();
	// Methods
	[[nodiscard]] std::pair<int, std::string> api_public(const std::string& msg);
	[[nodiscard]] std::pair<int, std::string> api_private(const std::string& msg);
	[[nodiscard]] int Authenticate();
private:
	Socket* m_socket;
	// Builds the auth payload from DERIBIT_CLIENT_ID / DERIBIT_CLIENT_SECRET
	// environment variables — no credential is ever hardcoded or committed.
	[[nodiscard]] std::string build_auth_msg() const;
};