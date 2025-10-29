#include "StompProtocol.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <iomanip>

using namespace std;

StompProtocol::StompProtocol(const std::string& host, int port)
    : connectionHandler(host, port),
      isConnected(false),
      shouldStop(false),
      subscriptionID(0),
      receiptID(0),
      subscriptions(),
      Events(),
      frameCreator() {}

StompProtocol::~StompProtocol() {
    disconnectFromServer();
}

std::string StompProtocol::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void StompProtocol::logMessage(const std::string& level, const std::string& message) {
    cout << "[" << getCurrentTimestamp() << "] [" << level << "] " << message << endl;
}

bool StompProtocol::connectToServer(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(connectionMutex);
    
    if (isConnected) {
        logMessage("ERROR", "Client already logged in. Log out before trying again.");
        return false;
    }
    if (!connectionHandler.connect()) {
        logMessage("ERROR", "Could not connect to server.");
        return false;
    }

    string connectFrame = frameCreator.createConnectFrame("stomp.cs.bgu.ac.il", username, password);

    if (!connectionHandler.sendFrameAscii(connectFrame, '\0')) {
        logMessage("ERROR", "Failed to send CONNECT frame to server.");
        return false;
    }
  

  //check from here
    string response;
    if (connectionHandler.getFrameAscii(response, '\0')) {
        if (response.find("CONNECTED") != string::npos) {
            this->username = username;
            logMessage("INFO", "User: " + username + " connected to server.");
            isConnected = true;
            shouldStop = false;
            return true;
        } else {
            logMessage("ERROR", "Server response did not indicate successful connection:\n" + response);
            return false;
        }
    } else {
        logMessage("ERROR", "Failed to receive response from server.");
        return false;
    }
}


void StompProtocol::disconnectFromServer() {
    //std::lock_guard<std::mutex> lock(connectionMutex);
    if (!isConnected) {
        logMessage("INFO", "No active session to disconnect. Ignoring request.");
        return;
    }

    shouldStop = true; // Signal the thread to stop

    string receiptId = to_string(++receiptID);
    string disconnectFrame = frameCreator.createDisconnectFrame(receiptId);

    if (!connectionHandler.sendFrameAscii(disconnectFrame, '\0')) {
        logMessage("ERROR", "Failed to send DISCONNECT frame to server.");
    }
    if (serverThread.joinable()) {
        serverThread.join(); // Ensure the message thread stops cleanly
    }

    connectionHandler.close();

    isConnected = false;
    {
        std::lock_guard<std::mutex> subLock(subscriptionsMutex);
        subscriptions.clear();
    }
    username.clear();
    logMessage("INFO", "User session disconnected.");
}


void StompProtocol::subscribeToTopic(const std::string& destination, const std::string& id) {
    if (!isConnected) {
        logMessage("ERROR", "Client not connected.");
        return;
    }

    std::lock_guard<std::mutex> lock(subscriptionsMutex);
    if (subscriptions.find(destination) != subscriptions.end()) {
        logMessage("ERROR", "Already subscribed to topic: " + destination);
        return;
    }

    string subscriptionId = to_string(++subscriptionID);
    string receiptId = to_string(++receiptID);
    string subscribeFrame = frameCreator.createSubscribeFrame(destination, subscriptionId, receiptId);
    if (connectionHandler.sendFrameAscii(subscribeFrame, '\0')) {
        subscriptions[destination] = std::stoi(subscriptionId);
        logMessage("INFO", "Subscribed to: " + destination);
    } else {
        logMessage("ERROR", "Failed to subscribe to topic: " + destination);
    }
}

void StompProtocol::unsubscribeFromTopic(const std::string& destination) {
    if (!isConnected) {
        logMessage("ERROR", "Client not connected.");
        return;
    }

    std::lock_guard<std::mutex> lock(subscriptionsMutex);
    auto it = subscriptions.find(destination);
    if (it == subscriptions.end()) {
        logMessage("ERROR", "Not subscribed to topic: " + destination);
        return;
    }

    string subscriptionId = to_string(it->second);
    string receiptId = to_string(++receiptID);
    string unsubscribeFrame = frameCreator.createUnsubscribeFrame(subscriptionId, receiptId);

    if (connectionHandler.sendFrameAscii(unsubscribeFrame, '\0')) {
        subscriptions.erase(it);
        logMessage("INFO", "Unsubscribed from: " + destination);
    } else {
        logMessage("ERROR", "Failed to unsubscribe from topic: " + destination);
    }
}
Event StompProtocol::parseEvent(const std::string& message) {
    // Define variables to hold the parsed data
    std::string city, name, description, channel, eventOwner;
    std::time_t date_time = 0;
    bool isActive = false, forcesArrival = false;
    std::map<std::string, std::string> additionalInfo;

    // Helper function to extract a field value by searching for the field name
    auto extractValueFromMessage = [](const std::string& msg, const std::string& fieldName) -> std::string {
        size_t fieldPos = msg.find(fieldName);
        if (fieldPos != std::string::npos) {
            size_t startPos = fieldPos + fieldName.length();
            size_t endPos = msg.find('\n', startPos);
            return msg.substr(startPos, endPos - startPos);
        }
        return "";
    };

    // Parse city, event name, description, date_time, and channel information from the message
    city = extractValueFromMessage(message, "city:");
    name = extractValueFromMessage(message, "event name:");
    description = extractValueFromMessage(message, "description:");

    // Parse event date_time (ensure valid format)
    std::string dateTimeStr = extractValueFromMessage(message, "date time:");
    if (!dateTimeStr.empty()) {
        try {
            date_time = std::stol(dateTimeStr);  // Convert to epoch time
        } catch (const std::exception& e) {
            logMessage("ERROR", "Invalid date_time format: " + dateTimeStr);
        }
    }

    // Parse active and forces arrival flags
    isActive = (message.find("active:true") != std::string::npos);
    forcesArrival = (message.find("forces_arrival_at_scene:true") != std::string::npos);

    // Add active and forces arrival to additionalInfo
    additionalInfo["active"] = isActive ? "true" : "false";
    additionalInfo["forces_arrival_at_scene"] = forcesArrival ? "true" : "false";

    // Extract general information if it exists
    std::string generalInfoSection = extractValueFromMessage(message, "general information:");
    if (!generalInfoSection.empty()) {
        std::istringstream infoStream(generalInfoSection);
        std::string line;
        while (std::getline(infoStream, line)) {
            size_t delimiterPos = line.find(':');
            if (delimiterPos != std::string::npos) {
                std::string key = line.substr(0, delimiterPos);
                std::string value = line.substr(delimiterPos + 1);
                additionalInfo[key] = value;
            }
        }
    }

    // Parse destination (channel)
    channel = extractValueFromMessage(message, "destination:");

    // Extract event owner user information from headers (if available)
    std::map<std::string, std::string> headers = frameCreator.parseMessage(message);
    eventOwner = headers.find("user") != headers.end() ? headers["user"] : "";
    Event parsedEvent(channel, city, name, date_time, description, additionalInfo);
    parsedEvent.setEventOwnerUser(eventOwner);

    return parsedEvent;
}



void StompProtocol::handleReceivedMessage(const std::string& message) {
    try {
        // Parse the raw message into headers and body
        auto headers = frameCreator.parseMessage(message);

        // Handle "ERROR" command
        if (headers.count("command") && headers.at("command") == "ERROR") {
            logMessage("ERROR", "Error response from server: " + message);
            return;
        }

        // Handle receipt acknowledgment
        if (headers.count("receipt-id")) {
            logMessage("INFO", "Receipt acknowledged: " + headers.at("receipt-id"));
            return;
        }

        // Handle "MESSAGE" command
        if (headers.count("command") && headers.at("command") == "MESSAGE") {
            logMessage("INFO", "Processing MESSAGE frame:\n" + message);

            // Extract required fields
            const std::string destination = headers.count("destination") ? headers.at("destination") : "";
            const std::string body = headers.count("body") ? headers.at("body") : "";

            if (destination.empty()) {
                logMessage("ERROR", "Missing 'destination' in MESSAGE frame.");
                return;
            }

            // Extract the user directly from the headers
            const std::string user = headers.count("user") ? headers.at("user") : "";

            if (user.empty()) {
                logMessage("ERROR", "Missing 'user' in MESSAGE frame.");
                return;
            }

            // Parse the body into event fields
            std::string channel_name, city, name, description;
            int date_time = 0; // Default to 0 if not provided
            std::map<std::string, std::string> general_information;

            // Assume parseEventBody is a helper function to extract fields from the body
            Event newEvent = parseEvent(message);;
            newEvent.setEventOwnerUser(user); // Set the event owner user directly from the header
            // Lock to ensure thread-safe modification of Events
            {
                std::lock_guard<std::mutex> lock(eventMutex);
                Events[destination].push_back(newEvent);
            }

            logMessage("INFO", "Event added to channel: " + destination);
            return;
        }

        // If the command is not recognized, log a warning
        logMessage("WARNING", "Unexpected server response: " + message);

    } catch (const std::out_of_range& e) {
        logMessage("ERROR", "Missing expected header key: " + std::string(e.what()));
    } catch (const std::exception& e) {
        logMessage("ERROR", "Failed to process server message: " + std::string(e.what()));
    }
}

void StompProtocol::sendMessage(const std::string& destination, const std::string& messageBody) {
    if (!isConnected) {
        logMessage("ERROR", "Client not connected.");
        return;
    }

    // Lock to ensure thread safety
    std::lock_guard<std::mutex> lock(subscriptionsMutex);

    // Check if subscribed to the destination
    if (subscriptions.find(destination) == subscriptions.end()) {
        logMessage("ERROR", "Not subscribed to topic: " + destination);
        return;
    }

    // Increment the receipt ID
    std::string receiptId = std::to_string(++receiptID);

    // Prepare the SEND frame using createSendFrame
    std::string sendFrame = frameCreator.createSendFrame("/"+destination, messageBody, receiptId);

    // Send the frame using the connection handler
    if (connectionHandler.sendFrameAscii(sendFrame, '\0')) {
        logMessage("INFO", "Message sent to: " + destination);
    } else {
        logMessage("ERROR", "Failed to send message to topic: " + destination);
    }
}


void StompProtocol::reportEvents(const std::string& filePath) {
    try {
        // Parse the events file
        names_and_events parsedData = parseEventsFile(filePath, username);
        const std::string& channelName = parsedData.channel_name;
        std::vector<Event>& parsedEvents = parsedData.events;

        // Sort events by date and time
        std::sort(parsedEvents.begin(), parsedEvents.end(), [](const Event& a, const Event& b) {
            return a.get_date_time() < b.get_date_time();
        });

        // Store events by channel
        Events[channelName] = parsedEvents;
        std::cout << "Events stored for channel: " << channelName << std::endl;

        // Loop through parsed events and send each with the correct user field
        for (auto& event : parsedEvents) {
            event.setEventOwnerUser(username);
            logMessage("INFO", "User set to: " + username + " for event: " + event.get_name()); 

            
            // Construct the message body for this event
            std::ostringstream messageBody;
            messageBody << "user:" << username << "\n"
                        << "channel name:" << channelName << "\n"
                        << "city:" << event.get_city() << "\n"
                        << "event name:" << event.get_name() << "\n"
                        << "date time:" << event.get_date_time() << "\n"
                        << "description:" << event.get_description() << "\n"
                        << "general information:\n";

            for (const auto& [key, value] : event.get_general_information()) {
                messageBody << "\t" << key << ":" << value << "\n";
            }

            // Send the constructed message
            sendMessage(channelName, messageBody.str());
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Failed to process emergency file: " << e.what() << "\n";
    }
}

std::string StompProtocol::epochToDate(time_t epochTime) {
    std::stringstream ss;
    ss << std::put_time(std::localtime(&epochTime), "%d/%m/%y %H:%M");
    return ss.str();
}

void StompProtocol::generateSummary(const std::string& channelName, const std::string& user, const std::string& filePath) {
    // Lock to ensure thread safety
    std::lock_guard<std::mutex> lock(eventMutex);

    // Check if the channel exists
    auto it = Events.find(channelName);
    if (it == Events.end()) {
        // Log error if the channel is not found
        logMessage("ERROR", "Channel not found: " + channelName);

        // First, modify channelName to add "/" at the beginning if not found
        std::string modifiedChannelName = "/" + channelName;
        it = Events.find(modifiedChannelName);

        if (it == Events.end()) {
            // If still not found, try removing "/" from the beginning (if it exists)
            if (!channelName.empty() && channelName[0] == '/') {
                modifiedChannelName = channelName.substr(1); // Remove the leading "/"
                it = Events.find(modifiedChannelName);
            }
        }

    if (it == Events.end()) {
        // If no valid channel is found, create a new one using the last modifiedChannelName
        logMessage("INFO", "Channel not found. Using default: " + modifiedChannelName);
        Events[modifiedChannelName]; // This creates a new entry for the modified channelName
        it = Events.find(modifiedChannelName); // Get the iterator to the newly added channel
    }
}

    const auto& events = it->second;

    // Filter events for the specified user
    std::vector<Event> userEvents;
    for (const auto& event : events) {
        if (event.getEventOwnerUser() == user) {
            userEvents.push_back(event);
        }
    }

    if (userEvents.empty()) {
        logMessage("INFO", "No events found for user: " + user + " in channel: " + channelName);
        return;
    }


    // Sort events by date and then by name
    std::sort(userEvents.begin(), userEvents.end(), [](const Event& a, const Event& b) {
        if (a.get_date_time() == b.get_date_time()) {
            return a.get_name() < b.get_name();
        }
        return a.get_date_time() < b.get_date_time();
    });

    // Calculate statistics
    int totalReports = static_cast<int>(userEvents.size());
    int activeCount = 0;
    int forcesArrivalCount = 0;

    for (const auto& event : userEvents) {
        if (event.isActive()) {
            activeCount++;
        }
        if (event.forcesArrivalAtScene()) {
            forcesArrivalCount++;
        }
    }

    // Construct the file path and open the file for writing
    std::string finalFilePath = "../bin/" + filePath;

    std::ofstream outFile(finalFilePath);
    if (!outFile.is_open()) {
        logMessage("ERROR", "Failed to open file for writing: " + finalFilePath);
        return;
    }

    // Write summary content to the file
    outFile << "Channel: " << channelName << "\n";
    outFile << "User: " << user << "\n";
    outFile << "Stats:\n";
    outFile << "Total: " << totalReports << "\n";
    outFile << "Active: " << activeCount << "\n";
    outFile << "Forces arrival at scene: " << forcesArrivalCount << "\n\n";

    outFile << "Event Reports:\n";
    for (const auto& event : userEvents) {
        outFile << epochToDate(event.get_date_time()) << " - " 
                << event.get_name() << " - " 
                << event.get_city() << ":\n";
        outFile << event.get_description() << "\n\n";
    }

    // Verify file write success
    if (!outFile) {
        logMessage("ERROR", "Failed to write data to file: " + finalFilePath);
    } else {
        logMessage("INFO", "Summary successfully written to: " + finalFilePath);
    }

    outFile.close();
}


void StompProtocol::runServerMessage() {
    
    while (!shouldStop) {
        {
            std::lock_guard<std::mutex> lock(connectionMutex);
            if (!isConnected) {
                logMessage("INFO", "Server connection is not active. Stopping message thread.");
                break;
            }
        }

        std::string frame;
        try {
            if (connectionHandler.getFrameAscii(frame, '\0')) {
                // Handle received messages
                handleReceivedMessage(frame);
            } else {
                std::lock_guard<std::mutex> lock(connectionMutex);
                isConnected = false;
                break; // Exit the loop on disconnection
            }
        } catch (const std::exception& e) {
            logMessage("ERROR", "Exception while processing server message: " + std::string(e.what()));
        } catch (...) {
            logMessage("ERROR", "Unknown exception caught in server message thread.");
        }
    }
}
