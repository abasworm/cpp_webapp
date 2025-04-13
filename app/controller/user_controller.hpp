#ifndef USER_CONTROLLER_HPP
#define USER_CONTROLLER_HPP

#include <string>
#include <unordered_map>
#include "../../json.hpp" // nlohmann json

// Use the nlohmann namespace
using json = nlohmann::json;

// Function declaration for handling user-specific requests
std::string handle_user_request(const std::string &method,
                                const std::string &path,
                                const std::unordered_map<std::string, std::string> &headers,
                                const std::string &body);

#endif // USER_CONTROLLER_HPP
