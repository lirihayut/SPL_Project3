#pragma once

#include "../include/ConnectionHandler.h"
#include <iostream>
#include <string>
#include <thread>
#include <mutex>

#ifndef STOMPPROTOCOL_H
#define STOMPPROTOCOL_H

#include <string>
#include <map>
#include "ConnectionHandler.h"
#include "CreateFrames.h"
#include "../include/event.h"


class StompProtocol {
private:
    ConnectionHandler connectionHandler;
    bool isConnected;
    std::map<std::string, int> subscriptions;
    std::map<std::string, std::vector<Event>> Events;
    int subscriptionID;
    int receiptID;
    CreateFrames frameCreator;
    std::string username;
    std::mutex connectionMutex;
    std::mutex subscriptionsMutex;
    std::mutex eventMutex;
    std::atomic<bool> shouldStop;
    std::thread serverThread;

public:
    StompProtocol(const std::string &host, int port);
    ~StompProtocol(); 
    std::string getCurrentTimestamp();
    void logMessage(const std::string &level, const std::string &message);
    bool connectToServer(const std::string &username, const std::string &password);
    void subscribeToTopic(const std::string& destination, const std::string& id);
    void sendMessage(const std::string& destination, const std::string& messageBody);
    void unsubscribeFromTopic(const std::string& id);
    void disconnectFromServer();
    Event parseEvent(const std::string &message);
    void handleReceivedMessage(const std::string &message);
    void reportEvents(const std::string &filePath);
    static std::string epochToDate(time_t epochTime);
    void generateSummary(const std::string &channelName, const std::string &user, const std::string &filePath);
    void runServerMessage();
};

#endif
