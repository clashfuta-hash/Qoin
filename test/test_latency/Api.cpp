#include "Api.hpp"
#include <cstdlib>
#include <stdexcept>
#include <nlohmann/json.hpp>

// Destructor
Api::~Api(){
	std::cout << "Destroying Api Instance\n";
	delete m_socket;
}

// Constructor
Api::Api(){
	// Create an instance of mysockets
	
	// Boost Socket Implementation
	//m_socket = new BSocket();

	// WebSocket++ Implementation
	m_socket = new Socketpp();	

	// Custom Implementation
	// m_socket = new CSocket();



	// Switch to WebSockets
	m_socket -> switch_to_ws();
	// Authenticate
	int status = Authenticate();
	if(status) {
		std::cout << "Authentication failed!\n";
		return;
	} else{
		std::cout << "Authentication Successful!\n";
	}
}

// sync public req
[[nodiscard]] std::pair<int, std::string> Api::api_public(const std::string& message){
	auto resp = m_socket -> ws_request(message);
	return resp;
}

// sync private req
[[nodiscard]] std::pair<int, std::string> Api::api_private(const std::string& message){
	auto resp = m_socket -> ws_request(message);
	return resp;
}

// async public req
void Api::api_public_async(const std::string& message){
	return m_socket -> ws_request_async(message, [this](int status, const std::string& resp) {
        	m_socket->ws_response_async(status, resp);
    	});
}

// async private req
void Api::api_private_async(const std::string& message){
	return m_socket -> ws_request_async(message, [this](int status, const std::string& resp) {
        	m_socket->ws_response_async(status, resp);
    	});
}

// Reads DERIBIT_CLIENT_ID and DERIBIT_CLIENT_SECRET from the environment
// and builds the auth payload at runtime. Nothing sensitive lives in the
// source code or gets committed to git.
[[nodiscard]] std::string Api::build_auth_msg() const {
	const char* client_id = std::getenv("DERIBIT_CLIENT_ID");
	const char* client_secret = std::getenv("DERIBIT_CLIENT_SECRET");

	if(!client_id || !client_secret) {
		throw std::runtime_error(
			"DERIBIT_CLIENT_ID and DERIBIT_CLIENT_SECRET must be set as "
			"environment variables before running this program.");
	}

	nlohmann::json auth_payload = {
		{"jsonrpc", "2.0"},
		{"id", 9929},
		{"method", "public/auth"},
		{"params", {
			{"grant_type", "client_credentials"},
			{"client_id", client_id},
			{"client_secret", client_secret}
		}}
	};

	return auth_payload.dump();
}

[[nodiscard]] int Api::Authenticate(){
	std::cout << "Logging in with client credentials\n";
	std::string auth_msg = build_auth_msg();
	std::pair<int, std::string> pr  = api_private(auth_msg);
	return pr.first;
}
