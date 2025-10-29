#include "ConnectionHandler.h"
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <map>


class StompClient {
private:
    ConnectionHandler connectionHandler;
    std::atomic<bool> isConnected;
    std::map<std::string, int> subscriptions; 
    int subscriptionCounterId;
    std::string username;

public:
    StompClient(const std::string& host, int port, const std::string& username) : connectionHandler(host, port), isConnected(false), subscriptions(), subscriptionCounterId(0), username(username) {}
};