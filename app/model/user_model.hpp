#ifndef USER_MODEL_HPP
#define USER_MODEL_HPP

#include <string>
#include "json.hpp" // nlohmann json

// Use the nlohmann namespace
using json = nlohmann::json;

// Function declarations for user database operations
// These now return json objects or indicate success/failure,
// response formatting happens elsewhere (controller or utils).
bool create_user_in_db(const std::string &username, const std::string &password, json &error_response);
json get_all_users_from_db(json &error_response); // Returns user array or empty array on error
bool delete_user_from_db(int id, json &error_response);

#endif // USER_MODEL_HPP
