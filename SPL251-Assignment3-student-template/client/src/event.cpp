#include "../include/event.h"
#include "../include/json.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <cstring>
#include <chrono>
#include "event.h"

using namespace std;
using json = nlohmann::json;

// Function to split a string by a delimiter
void split_str(const std::string &s, char delimiter, std::vector<std::string> &tokens) {
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
}

// Event Class Implementation

Event::Event(std::string channel_name, std::string city, std::string name, int date_time,
             std::string description, std::map<std::string, std::string> general_information)
    : channel_name(channel_name), city(city), name(name),
      date_time(date_time), description(description), general_information(general_information), eventOwnerUser("")
{
}

Event::~Event()
{
}

void Event::setEventOwnerUser(std::string setEventOwnerUser) {
    eventOwnerUser = setEventOwnerUser;
}

const std::string &Event::getEventOwnerUser() const {
    return eventOwnerUser;
    
}

const std::string &Event::get_channel_name() const {
    return this->channel_name;
}

const std::string &Event::get_city() const {
    return this->city;
}

const std::string &Event::get_name() const {
    return this->name;
}

int Event::get_date_time() const {
    return this->date_time;
}

const std::map<std::string, std::string> &Event::get_general_information() const {
    return this->general_information;
}

const std::string &Event::get_description() const {
    return this->description;
}

bool Event::isActive() const {
    auto it = general_information.find("active");
    return it != general_information.end() && it->second == "true";
}

bool Event::forcesArrivalAtScene() const {
    auto it = general_information.find("forces_arrival_at_scene");
    return it != general_information.end() && it->second == "true";
}

int Event::getCurrentTime() const {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto duration = now.time_since_epoch();
    int seconds = duration_cast<std::chrono::seconds>(duration).count();
    return static_cast<int>(seconds);
}

std::string Event::toString() const {
    std::ostringstream out;

    out << "user:" << eventOwnerUser << "\n";
    out << "channel name:" << channel_name << "\n";
    out << "city:" << city << "\n";
    out << "event name:" << name << "\n";
    out << "date time:" << date_time << "\n";
    out << "description:" << description << "\n";

    out << "general information:" << "\n";
    for (const auto& [key, value] : general_information) {
        out << "\t" << key << ":" << value << "\n";
    }

    return out.str();
}

Event::Event(const std::string &frame_body)
    : channel_name(""), city(""), name(""), date_time(0), description(""), general_information(), eventOwnerUser("")
{
    stringstream ss(frame_body);
    string line;
    string eventDescription;
    map<string, string> general_information_from_string;
    bool inGeneralInformation = false;
    while(getline(ss,line,'\n')) {
        vector<string> lineArgs;
        if(line.find(':') != string::npos) {
            split_str(line, ':', lineArgs);
            string key = lineArgs.at(0);
            string val;
            if(lineArgs.size() == 2) {
                val = lineArgs.at(1);
            }
            if(key == "user") {
                eventOwnerUser = val;
            }
            if(key == "channel name") {
                channel_name = val;
            }
            if(key == "city") {
                city = val;
            }
            else if(key == "event name") {
                name = val;
            }
            else if(key == "date time") {
                date_time = std::stoi(val);
            }
            else if(key == "general information") {
                inGeneralInformation = true;
                continue;
            }
            else if(key == "description") {
                while(getline(ss,line,'\n')) {
                    eventDescription += line + "\n";
                }
                description = eventDescription;
            }

            if(inGeneralInformation) {
                general_information_from_string[key.substr(1)] = val;
            }
        }
    }
    general_information = general_information_from_string;
}

// Helper function to parse events from a JSON file
names_and_events parseEventsFile(std::string json_path, const std::string& eventOwnerUser)
{
    std::ifstream f(json_path);
    json data = json::parse(f);

    std::string channel_name = data["channel_name"];

    // run over all the events and convert them to Event objects
    std::vector<Event> events;
    for (auto &event : data["events"]) {
        std::string name = event["event_name"];
        std::string city = event["city"];
        int date_time = event["date_time"];
        std::string description = event["description"];
        std::map<std::string, std::string> general_information;
        for (auto &update : event["general_information"].items()) {
            if (update.value().is_string())
                general_information[update.key()] = update.value();
            else
                general_information[update.key()] = update.value().dump();
        }

        events.push_back(Event(channel_name, city, name, date_time, description, general_information));
        events.back().setEventOwnerUser(eventOwnerUser); // Set user after creation

    }

    names_and_events events_and_names{channel_name, events};
    return events_and_names;
}
