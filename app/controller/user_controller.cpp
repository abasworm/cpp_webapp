#include "user_controller.hpp"     // Include the header file
#include "../model/user_model.hpp" // Include the model functions
#include "../config/utils.hpp"     // Include response formatting utils
#include <iostream>                // For std::cerr, std::cout

// Function to handle user-specific API requests
std::string handle_user_request(const std::string &method,
                                const std::string &path,
                                const std::unordered_map<std::string, std::string> &headers,
                                const std::string &body)
{
  // --- Routing Logic for /users endpoint ---

  if (path == "/users")
  {
    if (method == "GET")
    {
      // --- GET /users ---
      json error_resp;
      json users = get_all_users_from_db(error_resp);
      if (!error_resp.is_null())
      {
        // Error occurred in model
        return response_http(500, error_resp);
      }
      // Success
      return response_http(200, users);
    }
    else if (method == "POST")
    {
      // --- POST /users ---
      // Check Content-Type
      auto content_type_it = headers.find("content-type");
      if (content_type_it == headers.end() || content_type_it->second.find("application/json") == std::string::npos)
      {
        return response_http(415, "Content-Type must be application/json");
      }
      if (body.empty())
      {
        return response_http(400, "Request body is empty");
      }

      try
      {
        json request_json = json::parse(body);
        if (!request_json.contains("username") || !request_json["username"].is_string() ||
            !request_json.contains("password") || !request_json["password"].is_string())
        {
          return response_http(400, "Missing or invalid 'username' or 'password' in JSON body");
        }

        std::string username = request_json["username"];
        std::string password = request_json["password"];

        if (username.empty() || password.empty())
        {
          return response_http(400, "'username' and 'password' cannot be empty");
        }

        // Call model function to create user
        json error_resp;
        bool success = create_user_in_db(username, password, error_resp);

        if (success)
        {
          return response_http(201, json{{"message", "User created successfully"}});
        }
        else
        {
          // Check error type from model
          if (error_resp.contains("error") && error_resp["error"] == "Username already exists")
          {
            return response_http(409, error_resp); // 409 Conflict
          }
          else
          {
            return response_http(500, error_resp); // 500 Internal Server Error
          }
        }
      }
      catch (json::parse_error &e)
      {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return response_http(400, "Invalid JSON format");
      }
      catch (json::type_error &e)
      {
        std::cerr << "JSON type error: " << e.what() << std::endl;
        return response_http(400, "Invalid JSON structure or types");
      }
      catch (const std::exception &e)
      {
        std::cerr << "Error processing POST /users: " << e.what() << std::endl;
        return response_http(500, "Internal server error processing request");
      }
    }
  }
  else if (path.rfind("/users/", 0) == 0)
  { // Check prefix for /users/{id}
    if (method == "DELETE")
    {
      // --- DELETE /users/{id} ---
      try
      {
        std::string id_str = path.substr(7); // Get ID part
        if (id_str.empty())
        {
          return response_http(400, "User ID missing in path");
        }
        int id = std::stoi(id_str);

        // Call model function to delete user
        json error_resp;
        bool success = delete_user_from_db(id, error_resp);

        if (success)
        {
          return response_http(200, json{{"message", "User deleted successfully"}});
        }
        else
        {
          // Check error type from model
          if (error_resp.contains("error") && error_resp["error"] == "User not found")
          {
            return response_http(404, error_resp); // 404 Not Found
          }
          else
          {
            return response_http(500, error_resp); // 500 Internal Server Error
          }
        }
      }
      catch (const std::invalid_argument &ia)
      {
        return response_http(400, "Invalid user ID format");
      }
      catch (const std::out_of_range &oor)
      {
        return response_http(400, "User ID out of range");
      }
      catch (const std::exception &e)
      {
        std::cerr << "Error processing DELETE /users/:id : " << e.what() << std::endl;
        return response_http(500, "Internal server error processing request");
      }
    }
  }

  // If none of the user routes matched
  return response_http(404, "User API endpoint not found");
}
