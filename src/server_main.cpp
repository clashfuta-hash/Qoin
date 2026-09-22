#include <drogon/drogon.h>
#include <mutex>
#include <cstdlib>
#include "Trader.hpp"

// The underlying Trader/Api/BSocket stack holds ONE blocking WebSocket
// connection to Deribit. It was written for a single-threaded CLI menu,
// not concurrent HTTP requests -- so every call into it is serialized
// through this mutex. Two requests hitting the socket at once would
// otherwise interleave writes/reads and corrupt each other's responses.
static std::mutex g_trader_mutex;
static Trader* g_trader = nullptr;

int main(){
	// One Trader instance for the whole server's lifetime. Its constructor
	// (via Api's constructor) authenticates immediately using
	// DERIBIT_CLIENT_ID / DERIBIT_CLIENT_SECRET from the environment --
	// make sure both are set before starting the server, or startup will
	// throw.
	g_trader = new Trader();

	// GET /api/orderbook?instrument=BTC-PERPETUAL&depth=10
	drogon::app().registerHandler(
		"/api/orderbook",
		[](const drogon::HttpRequestPtr &req,
		   std::function<void(const drogon::HttpResponsePtr &)> &&callback){
			auto instrument = req->getParameter("instrument");
			auto depthParam = req->getParameter("depth");
			int depth = depthParam.empty() ? 10 : std::stoi(depthParam);

			if(instrument.empty()){
				auto resp = drogon::HttpResponse::newHttpJsonResponse(
					Json::Value("instrument query parameter is required"));
				resp->setStatusCode(drogon::k400BadRequest);
				callback(resp);
				return;
			}

			std::pair<int,std::string> result;
			{
				std::lock_guard<std::mutex> lock(g_trader_mutex);
				result = g_trader->get_orderbook(instrument, depth);
			}

			auto resp = drogon::HttpResponse::newHttpResponse();
			resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
			resp->setBody(result.second);
			resp->setStatusCode(result.first == 0 ? drogon::k200OK
			                                       : drogon::k502BadGateway);
			callback(resp);
		},
		{drogon::Get});

	// GET /api/positions?instrument=BTC-PERPETUAL
	drogon::app().registerHandler(
		"/api/positions",
		[](const drogon::HttpRequestPtr &req,
		   std::function<void(const drogon::HttpResponsePtr &)> &&callback){
			auto instrument = req->getParameter("instrument");
			if(instrument.empty()){
				auto resp = drogon::HttpResponse::newHttpJsonResponse(
					Json::Value("instrument query parameter is required"));
				resp->setStatusCode(drogon::k400BadRequest);
				callback(resp);
				return;
			}

			std::pair<int,std::string> result;
			{
				std::lock_guard<std::mutex> lock(g_trader_mutex);
				result = g_trader->view_position(instrument);
			}

			auto resp = drogon::HttpResponse::newHttpResponse();
			resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
			resp->setBody(result.second);
			resp->setStatusCode(result.first == 0 ? drogon::k200OK
			                                       : drogon::k502BadGateway);
			callback(resp);
		},
		{drogon::Get});

	// GET /api/orders?kind=null   (open orders)
	drogon::app().registerHandler(
		"/api/orders",
		[](const drogon::HttpRequestPtr &req,
		   std::function<void(const drogon::HttpResponsePtr &)> &&callback){
			auto kind = req->getParameter("kind");
			if(kind.empty()) kind = "null";

			std::pair<int,std::string> result;
			{
				std::lock_guard<std::mutex> lock(g_trader_mutex);
				result = g_trader->get_openorders(kind);
			}

			auto resp = drogon::HttpResponse::newHttpResponse();
			resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
			resp->setBody(result.second);
			resp->setStatusCode(result.first == 0 ? drogon::k200OK
			                                       : drogon::k502BadGateway);
			callback(resp);
		},
		{drogon::Get});

	// POST /api/orders   body: {"instrument": "...", "price": 100.0, "quantity": 1}
	drogon::app().registerHandler(
		"/api/orders",
		[](const drogon::HttpRequestPtr &req,
		   std::function<void(const drogon::HttpResponsePtr &)> &&callback){
			auto json = req->getJsonObject();
			if(!json || !json->isMember("instrument") ||
			   !json->isMember("price") || !json->isMember("quantity")){
				auto resp = drogon::HttpResponse::newHttpJsonResponse(
					Json::Value("instrument, price, and quantity are required"));
				resp->setStatusCode(drogon::k400BadRequest);
				callback(resp);
				return;
			}

			std::string instrument = (*json)["instrument"].asString();
			double price = (*json)["price"].asDouble();
			int quantity = (*json)["quantity"].asInt();

			std::pair<int,std::string> result;
			{
				std::lock_guard<std::mutex> lock(g_trader_mutex);
				result = g_trader->place_order(instrument, price, quantity);
			}

			auto resp = drogon::HttpResponse::newHttpResponse();
			resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
			resp->setBody(result.second);
			resp->setStatusCode(result.first == 0 ? drogon::k200OK
			                                       : drogon::k502BadGateway);
			callback(resp);
		},
		{drogon::Post});

	// DELETE /api/orders/{order_id}
	drogon::app().registerHandler(
		"/api/orders/{1}",
		[](const drogon::HttpRequestPtr &req,
		   std::function<void(const drogon::HttpResponsePtr &)> &&callback,
		   std::string orderId){
			std::pair<int,std::string> result;
			{
				std::lock_guard<std::mutex> lock(g_trader_mutex);
				result = g_trader->cancel_order(orderId);
			}

			auto resp = drogon::HttpResponse::newHttpResponse();
			resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
			resp->setBody(result.second);
			resp->setStatusCode(result.first == 0 ? drogon::k200OK
			                                       : drogon::k502BadGateway);
			callback(resp);
		},
		{drogon::Delete});

	// PATCH /api/orders/{order_id}   body: {"price": 100.0, "quantity": 1}
	drogon::app().registerHandler(
		"/api/orders/{1}",
		[](const drogon::HttpRequestPtr &req,
		   std::function<void(const drogon::HttpResponsePtr &)> &&callback,
		   std::string orderId){
			auto json = req->getJsonObject();
			if(!json || !json->isMember("price") || !json->isMember("quantity")){
				auto resp = drogon::HttpResponse::newHttpJsonResponse(
					Json::Value("price and quantity are required"));
				resp->setStatusCode(drogon::k400BadRequest);
				callback(resp);
				return;
			}

			double price = (*json)["price"].asDouble();
			int quantity = (*json)["quantity"].asInt();

			std::pair<int,std::string> result;
			{
				std::lock_guard<std::mutex> lock(g_trader_mutex);
				result = g_trader->modify_order(orderId, price, quantity);
			}

			auto resp = drogon::HttpResponse::newHttpResponse();
			resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
			resp->setBody(result.second);
			resp->setStatusCode(result.first == 0 ? drogon::k200OK
			                                       : drogon::k502BadGateway);
			callback(resp);
		},
		{drogon::Patch});

	// Render injects PORT; default to 8080 for local runs.
	const char* portEnv = std::getenv("PORT");
	int port = portEnv ? std::stoi(portEnv) : 8080;

	LOG_INFO << "Qoin backend listening on port " << port;
	drogon::app().addListener("0.0.0.0", port).run();

	delete g_trader;
	return 0;
}
