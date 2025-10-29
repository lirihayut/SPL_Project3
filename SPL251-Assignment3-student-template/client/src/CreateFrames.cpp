#include "../include/CreateFrames.h"
#include <sstream>
#include <iostream>
#include "CreateFrames.h"

CreateFrames::CreateFrames() : terminate(false) {}
CreateFrames::~CreateFrames() {}

std::string CreateFrames::createConnectFrame(const std::string& host, const std::string& username, const std::string& passcode) {
    std::ostringstream frame;
    frame << "CONNECT\n"
          << "accept-version:1.2\n"
          << "host:" << host << "\n"
          << "login:" << username << "\n"
          << "passcode:" << passcode << "\n\n";
    return frame.str();
}


std::string CreateFrames::createSubscribeFrame(const std::string& destination, const std::string& id, const std::string& receiptId) {
    std::ostringstream frame;
    frame << "SUBSCRIBE\n"
          << "destination:" << destination << "\n"
          << "id:" << id << "\n"
          << "receipt:" << receiptId << "\n\n";
    return frame.str();
}

std::string CreateFrames::createUnsubscribeFrame(const std::string& id, const std::string& receiptId) {
    std::ostringstream frame;
    frame << "UNSUBSCRIBE\n"
          << "id:" << id << "\n"
          << "receipt:" << receiptId << "\n\n";
    return frame.str();
}

std::string CreateFrames::createSendFrame(const std::string& destination, const std::string& body, const std::string& receiptId) {
    std::ostringstream frame;

    // Extract 'user' from the body (assuming the body contains 'user:<username>')
    std::string user;
    size_t userPos = body.find("user:");
    if (userPos != std::string::npos) {
        user = body.substr(userPos + 5);  // 5 is the length of "user:"
        size_t endPos = user.find(';');    // Assuming user ends with a semicolon
        if (endPos != std::string::npos) {
            user = user.substr(0, endPos);  // Extract the username part
        }
    }

    // Create the frame with the 'user' header if found
    frame << "SEND\n"
          << "destination:" << destination << "\n"
          << "receipt:" << receiptId << "\n";
    
    // Add 'user' to the frame header if it's extracted
    if (!user.empty()) {
        frame << "user:" << user << "\n";
    }

    // Add the body
    frame << "\n" << body << "\n";

    return frame.str();
}



std::string CreateFrames::createDisconnectFrame(const std::string& receiptId) {
    std::ostringstream frame;
    frame << "DISCONNECT\n"
          << "receipt:" << receiptId << "\n\n";
    return frame.str();
}

std::map<std::string, std::string> CreateFrames::parseMessage(const std::string& message) {
    std::map<std::string, std::string> headers;
    std::istringstream stream(message);
    std::string line;

    // Extract command
    if (std::getline(stream, line)) {
        headers["command"] = line;
    } else {
        std::cerr << "[ERROR] Empty response received." << std::endl;
        return headers;
    }

    // Extract headers
    while (std::getline(stream, line) && !line.empty()) {
        auto separator = line.find(':');
        if (separator != std::string::npos) {
            std::string key = line.substr(0, separator);
            std::string value = line.substr(separator + 1);
            headers[key] = value;
        } else {
            std::cerr << "[WARNING] Malformed header: " << line << std::endl;
        }
    }

    return headers;
}

bool CreateFrames::shouldTerminate() const {
    return terminate;
}
