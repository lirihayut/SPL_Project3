#ifndef STOMP_PROTOCOL_H
#define STOMP_PROTOCOL_H

#include <string>
#include <map>

class CreateFrames {
public:
    // Constructor and Destructor
    CreateFrames();
    ~CreateFrames();

    // Frame creation methods
    std::string createConnectFrame(const std::string& host, const std::string& login, const std::string& passcode);
    std::string createSubscribeFrame(const std::string& destination, const std::string& id, const std::string& receiptId = "");
    std::string createUnsubscribeFrame(const std::string& id, const std::string& receiptId = "");
    std::string createSendFrame(const std::string& destination, const std::string& body, const std::string& receiptId = "");
    std::string createDisconnectFrame(const std::string& receiptId = "");

    // Message parsing and utilities
    std::map<std::string, std::string> parseMessage(const std::string& message);
    void logFrame(const std::string& frame) const; // Const-correctness for read-only method
    bool shouldTerminate() const;

    // Utility to set the termination flag
    void setTerminate(bool value);

private:
    bool terminate = false; // Properly initialize member variable
};

#endif // STOMP_PROTOCOL_H
