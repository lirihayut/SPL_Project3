#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include "StompProtocol.h" 

using namespace std;

int main(int argc, char* argv[]) {
    cout << "[INFO] Welcome to StompEMIClient!" << endl;

     std::atomic<bool> running(true);
    bool isLogin = false;
     std::thread inputThread;
     std::thread serverThread;
    StompProtocol* protocol = nullptr;

    if (argc == 1) {
        while (running) {
            cout << "> ";
            string line;
            getline(cin, line);

            istringstream ss(line);
            string command;
            ss >> command;

            if (command == "login") {
                string hostWithPort, username, password;
                if (!(ss >> hostWithPort >> username >> password)) {
                    cerr << "[ERROR] Invalid login format! Use: login <host:port> <username> <password>" << endl;
                    continue;
                }

                size_t colonPos = hostWithPort.find(':');
                if (colonPos == string::npos) {
                    cerr << "[ERROR] Invalid host:port format!" << endl;
                    continue;
                }

                string host = hostWithPort.substr(0, colonPos);
                int port = stoi(hostWithPort.substr(colonPos + 1));

                if (protocol) {
                    cerr << "[ERROR] Already logged in. Please logout first." << endl;
                    continue;
                }

                protocol = new StompProtocol(host, port);
                if (!protocol->connectToServer(username, password)) {
                    cerr << "[ERROR] Login failed." << endl;
                    delete protocol;
                    protocol = nullptr;
                    continue;
                }

                if (!isLogin)
                {
                    isLogin = true;
                    serverThread = std::thread([&protocol]() { protocol->runServerMessage(); });
                }
            } else if (command == "logout") {
                if (!protocol) {
                    cerr << "[ERROR] Not logged in." << endl;
                    continue;
                }

                protocol->disconnectFromServer();
                delete protocol;
                protocol = nullptr;
                cout << "[INFO] Logged out successfully." << endl;
            } else if (command == "join") {
                if (!protocol) {
                    cerr << "[ERROR] Not logged in. Please login first." << endl;
                    continue;
                }

                string channel;
                ss >> channel;
                if (channel.empty()) {
                    cerr << "[ERROR] No channel specified. Use: join <channel>" << endl;
                    continue;
                }

                static int subscriptionID = 0;
                protocol->subscribeToTopic(channel, to_string(subscriptionID++));
            } else if (command == "report") {
                if (!protocol) {
                    cerr << "[ERROR] Not logged in. Please login first." << endl;
                    continue;
                }

                string filePath;
                ss >> filePath;
                if (filePath.empty()) {
                    cerr << "[ERROR] No file path specified. Use: report <file_path>" << endl;
                    continue;
                }

                protocol->reportEvents(filePath);
            } else if (command == "summary") {
                if (!protocol) {
                    cerr << "[ERROR] Not logged in. Please login first." << endl;
                    continue;
                }

                string channel, user, filePath;
                ss >> channel >> user >> filePath;
                if (channel.empty() || user.empty() || filePath.empty()) {
                    cerr << "[ERROR] Missing arguments. Use: summary <channel> <user> <file_path>" << endl;
                    continue;
                }

                protocol->generateSummary(channel, user, filePath);
            } else if (command == "exit") {
                 if (!protocol) {
                    cerr << "[ERROR] Not logged in. Please login first." << endl;
                    continue;
                }

                string channel;
                ss >> channel;
                if (channel.empty()) {
                    cerr << "[ERROR] No channel specified. Use: exit <channel>" << endl;
                    continue;
                }
                protocol->unsubscribeFromTopic(channel);
            } else {
                cerr << "[ERROR] Unknown command: " << command << endl;
            }
        }
    } else {
        cerr << "[ERROR] Invalid usage. No arguments expected." << endl;
        return -1;
    }

        if (protocol != nullptr) {
        serverThread.join();
        delete protocol;
    }
    cout << "Client terminated gracefully." << endl;
    return 0;

}
