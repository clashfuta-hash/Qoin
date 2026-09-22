#pragma once
#include <iostream>
#include <cstdint>
#include <memory>
#include <functional>
#include <sstream>

// Renamed from "Json" -- that name collided with the real jsoncpp
// library's `namespace Json`, which Drogon depends on. This struct is
// unused elsewhere in the codebase, so the rename changes nothing
// functionally.
struct RpcMessage{
	std::string method;
	std::string params;
	std::string id;
	const std::string jsonrpc = "2.0";
};
