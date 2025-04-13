#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <map>
#include "../json.hpp" // nlohmann json

// Use the nlohmann namespace
using json = nlohmann::json;

// --- HTTP Response Helper ---

// Declare the map for external linkage
extern const std::map<int, std::string> http_status_reasons;

// Function declarations
std::string response_http(int status_code, const json &data, const std::string &content_type = "application/json");
std::string response_http(int status_code, const std::string &error_message);

// --- SHA256 Helper ---
std::string sha256(const std::string &str);

#endif // UTILS_HPP
