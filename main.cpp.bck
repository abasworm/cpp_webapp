// main.cpp
#include <iostream>
#include <string>
#include <unordered_map> // For headers
#include <map>           // For status code map
#include <thread>
#include <sstream>
#include <netinet/in.h>
#include <unistd.h>
#include <sqlite3.h>
#include <openssl/sha.h>
#include <vector>
#include <algorithm> // For std::transform (lowercase header names)
#include <cctype>    // For std::tolower
#include <iomanip>   // For std::setw, std::setfill

// Include the nlohmann JSON library header
#include "json.hpp"

// Use the nlohmann namespace for convenience
using json = nlohmann::json;

#define PORT 8080
#define READ_BUFFER_SIZE 4096 // Increased buffer size for potentially larger requests

// Global database connection pointer
sqlite3 *db;

// --- HTTP Response Helper ---

// Map status codes to reason phrases
const std::map<int, std::string> http_status_reasons = {
    {200, "OK"},
    {201, "Created"},
    {400, "Bad Request"},
    {404, "Not Found"},
    {409, "Conflict"},
    {415, "Unsupported Media Type"},
    {500, "Internal Server Error"}
    // Add more status codes as needed
};

/**
 * @brief Constructs an HTTP response string.
 *
 * @param status_code The HTTP status code (e.g., 200, 404).
 * @param data The JSON data to include in the response body.
 * @param content_type The Content-Type header value (defaults to application/json).
 * @return The fully formatted HTTP response string.
 */
std::string response_http(int status_code, const json &data, const std::string &content_type = "application/json")
{
  std::string reason_phrase = "Unknown Status";
  // Find the reason phrase for the status code
  auto it = http_status_reasons.find(status_code);
  if (it != http_status_reasons.end())
  {
    reason_phrase = it->second;
  }

  // Convert JSON data to string body
  std::string body = data.dump();
  std::ostringstream oss; // Use output string stream for easy construction

  // Status Line
  oss << "HTTP/1.1 " << status_code << " " << reason_phrase << "\r\n";

  // Headers
  oss << "Content-Type: " << content_type << "\r\n";
  oss << "Content-Length: " << body.length() << "\r\n";
  // Add other headers like Server, Date if desired
  // oss << "Server: CppWebServer/1.0\r\n";

  // End of Headers
  oss << "\r\n";

  // Body
  oss << body;

  return oss.str();
}

// Overload for simple error messages (creates a JSON error object)
std::string response_http(int status_code, const std::string &error_message)
{
  json error_json = {{"error", error_message}};
  return response_http(status_code, error_json);
}

// --- SHA256 ---
std::string sha256(const std::string &str)
{
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char *>(str.c_str()), str.size(), hash);
  std::ostringstream oss;
  for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
  {
    oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
  }
  return oss.str();
}

// --- DB Init ---
void init_db()
{
  const char *sql = "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, username TEXT UNIQUE, password TEXT);";
  char *err = nullptr;
  if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK)
  {
    std::cerr << "Database error during init: " << err << std::endl;
    sqlite3_free(err);
    // Consider exiting or handling this more gracefully
  }
}

// --- DB Operations (Refactored to use response_http) ---

// Function to create a new user
std::string create_user(const std::string &username, const std::string &password)
{
  std::string hash = sha256(password);
  sqlite3_stmt *stmt;
  std::string sql = "INSERT INTO users (username, password) VALUES (?, ?);";

  // Prepare statement
  if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
  {
    std::cerr << "DB prepare error (create_user): " << sqlite3_errmsg(db) << std::endl;
    json error_json = {{"error", "Database preparation failed"}};
    return response_http(500, error_json);
  }

  // Bind parameters
  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_STATIC);

  // Execute statement
  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt); // Finalize statement

  // Handle execution result
  if (rc != SQLITE_DONE)
  {
    if (rc == SQLITE_CONSTRAINT)
    {
      // Username already exists
      json error_json = {{"error", "Username already exists"}};
      return response_http(409, error_json);
    }
    else
    {
      // Other database error
      std::cerr << "DB execution error (create_user): " << sqlite3_errmsg(db) << std::endl;
      json error_json = {{"error", "Failed to create user"}};
      return response_http(500, error_json);
    }
  }

  // Success
  json success_msg = {{"message", "User created successfully"}};
  return response_http(201, success_msg);
}

// Function to retrieve all users
std::string get_users()
{
  sqlite3_stmt *stmt;
  std::string sql = "SELECT id, username FROM users;";
  json users_json = json::array(); // Create a JSON array

  // Prepare statement
  if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
  {
    // Fetch rows
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
      json user;
      user["id"] = sqlite3_column_int(stmt, 0);
      const unsigned char *username_text = sqlite3_column_text(stmt, 1);
      user["username"] = username_text ? reinterpret_cast<const char *>(username_text) : "";
      users_json.push_back(user);
    }
    sqlite3_finalize(stmt); // Finalize statement
  }
  else
  {
    // DB error during preparation
    std::cerr << "DB prepare error (get_users): " << sqlite3_errmsg(db) << std::endl;
    json error_json = {{"error", "Failed to retrieve users"}};
    return response_http(500, error_json);
  }

  // Success - return the JSON array
  return response_http(200, users_json);
}

// Function to delete a user by ID
std::string delete_user(int id)
{
  sqlite3_stmt *stmt;
  std::string sql = "DELETE FROM users WHERE id = ?;";

  // Prepare statement
  if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
  {
    std::cerr << "DB prepare error (delete_user): " << sqlite3_errmsg(db) << std::endl;
    json error_json = {{"error", "Database preparation failed"}};
    return response_http(500, error_json);
  }

  // Bind ID
  sqlite3_bind_int(stmt, 1, id);

  // Execute statement
  int rc = sqlite3_step(stmt);
  int changes = sqlite3_changes(db); // Get affected rows count *before* finalizing
  sqlite3_finalize(stmt);            // Finalize statement

  // Check if user was found and deleted
  if (changes == 0)
  {
    json error_json = {{"error", "User not found"}};
    return response_http(404, error_json);
  }

  // Check for execution errors (other than not found)
  if (rc != SQLITE_DONE)
  {
    std::cerr << "DB execution error (delete_user): " << sqlite3_errmsg(db) << std::endl;
    json error_json = {{"error", "Failed to delete user"}};
    return response_http(500, error_json);
  }

  // Success
  json success_msg = {{"message", "User deleted successfully"}};
  return response_http(200, success_msg);
}

// --- Request Handling (Refactored to use response_http) ---

// Function to parse request and route to appropriate handler
std::string handle_request(const std::string &method,
                           const std::string &path,
                           const std::unordered_map<std::string, std::string> &headers,
                           const std::string &body)
{
  std::cout << "Received Request: " << method << " " << path << std::endl;

  // --- Routing ---
  if (method == "GET" && path == "/users")
  {
    return get_users(); // Already returns formatted response
  }
  else if (method == "POST" && path == "/users")
  {
    // Check Content-Type
    auto content_type_it = headers.find("content-type");
    if (content_type_it == headers.end() || content_type_it->second.find("application/json") == std::string::npos)
    {
      json error_json = {{"error", "Content-Type must be application/json"}};
      return response_http(415, error_json);
    }

    // Check for empty body
    if (body.empty())
    {
      json error_json = {{"error", "Request body is empty"}};
      return response_http(400, error_json);
    }

    try
    {
      // Parse JSON body
      json request_json = json::parse(body);

      // Validate required fields exist and are strings
      if (!request_json.contains("username") || !request_json["username"].is_string() ||
          !request_json.contains("password") || !request_json["password"].is_string())
      {
        json error_json = {{"error", "Missing or invalid 'username' or 'password' in JSON body"}};
        return response_http(400, error_json);
      }

      // Extract values
      std::string username = request_json["username"];
      std::string password = request_json["password"];

      // Validate values are not empty
      if (username.empty() || password.empty())
      {
        json error_json = {{"error", "'username' and 'password' cannot be empty"}};
        return response_http(400, error_json);
      }

      // Create user (already returns formatted response)
      return create_user(username, password);
    }
    catch (json::parse_error &e)
    {
      std::cerr << "JSON parse error: " << e.what() << std::endl;
      json error_json = {{"error", "Invalid JSON format"}};
      return response_http(400, error_json);
    }
    catch (json::type_error &e)
    {
      std::cerr << "JSON type error: " << e.what() << std::endl;
      json error_json = {{"error", "Invalid JSON structure or types"}};
      return response_http(400, error_json);
    }
    catch (const std::exception &e)
    {
      std::cerr << "Error processing POST /users: " << e.what() << std::endl;
      json error_json = {{"error", "Internal server error processing request"}};
      return response_http(500, error_json);
    }
  }
  else if (method == "DELETE" && path.rfind("/users/", 0) == 0) // Check prefix
  {
    try
    {
      std::string id_str = path.substr(7); // Get substring after "/users/"
      if (id_str.empty())
      {
        json error_json = {{"error", "User ID missing in path"}};
        return response_http(400, error_json);
      }
      // Convert ID string to integer
      int id = std::stoi(id_str);
      // Delete user (already returns formatted response)
      return delete_user(id);
    }
    catch (const std::invalid_argument &ia)
    {
      // stoi failed - invalid format
      json error_json = {{"error", "Invalid user ID format"}};
      return response_http(400, error_json);
    }
    catch (const std::out_of_range &oor)
    {
      // stoi failed - ID out of range
      json error_json = {{"error", "User ID out of range"}};
      return response_http(400, error_json);
    }
    catch (const std::exception &e)
    {
      std::cerr << "Error processing DELETE /users/:id : " << e.what() << std::endl;
      return response_http(500, json{{"error", "Internal server error processing request"}});
    }
  }

  // Default fallback for unhandled routes
  return response_http(404, json{{"error", "Resource not found"}});
}

// --- Client Handling (Mostly Unchanged Internally) ---
void handle_client(int client_socket)
{
  char buffer[READ_BUFFER_SIZE] = {0};
  std::string request_data;
  ssize_t bytes_read;

  // Read initial data
  bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
  if (bytes_read <= 0)
  {
    if (bytes_read < 0)
      perror("read failed");
    std::cerr << "Failed to read from socket or connection closed cleanly." << std::endl;
    close(client_socket);
    return;
  }
  buffer[bytes_read] = '\0';
  request_data.append(buffer, bytes_read);

  // Find end of headers
  size_t headers_end_pos = request_data.find("\r\n\r\n");
  if (headers_end_pos == std::string::npos)
  {
    std::cerr << "Could not find end of HTTP headers." << std::endl;
    // Send basic error response (not using response_http as request is malformed)
    std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 28\r\n\r\nMalformed request headers.";
    send(client_socket, response.c_str(), response.length(), 0);
    close(client_socket);
    return;
  }

  // Separate headers and initial body part
  std::string headers_str = request_data.substr(0, headers_end_pos);
  std::string body_part = request_data.substr(headers_end_pos + 4);

  // Parse request line
  std::istringstream headers_stream(headers_str);
  std::string request_line;
  std::getline(headers_stream, request_line);
  if (!request_line.empty() && request_line.back() == '\r')
  {
    request_line.pop_back();
  }

  std::string method, path, version;
  std::istringstream request_line_stream(request_line);
  request_line_stream >> method >> path >> version;

  // Parse headers
  std::unordered_map<std::string, std::string> headers;
  std::string header_line;
  size_t content_length = 0;

  while (std::getline(headers_stream, header_line))
  {
    if (!header_line.empty() && header_line.back() == '\r')
    {
      header_line.pop_back();
    }
    if (header_line.empty())
      break; // End of headers

    size_t colon_pos = header_line.find(':');
    if (colon_pos != std::string::npos)
    {
      std::string header_name = header_line.substr(0, colon_pos);
      std::string header_value = header_line.substr(colon_pos + 1);

      // Trim whitespace from value
      size_t first = header_value.find_first_not_of(" \t");
      if (first != std::string::npos)
      {
        size_t last = header_value.find_last_not_of(" \t");
        header_value = header_value.substr(first, (last - first + 1));
      }
      else
      {
        header_value = "";
      }

      // Store header name lowercase
      std::transform(header_name.begin(), header_name.end(), header_name.begin(),
                     [](unsigned char c)
                     { return std::tolower(c); });
      headers[header_name] = header_value;

      // Extract Content-Length
      if (header_name == "content-length")
      {
        try
        {
          content_length = std::stoul(header_value);
        }
        catch (const std::exception &e)
        {
          std::cerr << "Invalid Content-Length value: " << header_value << std::endl;
          // Send error (not using response_http as request is malformed)
          std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 29\r\n\r\nInvalid Content-Length header.";
          send(client_socket, response.c_str(), response.length(), 0);
          close(client_socket);
          return;
        }
      }
    }
  }

  // Construct full body
  std::string body = body_part;
  if (content_length > 0 && body.length() < content_length)
  {
    size_t remaining_length = content_length - body.length();
    std::vector<char> body_buffer(remaining_length);
    ssize_t body_bytes_read = read(client_socket, body_buffer.data(), remaining_length);

    if (body_bytes_read > 0)
    {
      body.append(body_buffer.data(), body_bytes_read);
    }
    else
    {
      if (body_bytes_read < 0)
        perror("read body failed");
      std::cerr << "Failed to read full request body." << std::endl;
      // Send error (not using response_http as request is incomplete)
      std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 26\r\n\r\nIncomplete request body.";
      send(client_socket, response.c_str(), response.length(), 0);
      close(client_socket);
      return;
    }
    if (body.length() < content_length)
    {
      std::cerr << "Incomplete request body received after read." << std::endl;
      std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 26\r\n\r\nIncomplete request body.";
      send(client_socket, response.c_str(), response.length(), 0);
      close(client_socket);
      return;
    }
  }
  else if (body.length() > content_length)
  {
    // Truncate if too much was read (e.g., pipelining)
    body.resize(content_length);
  }

  // Handle the request (this now uses the refactored functions)
  std::string response = handle_request(method, path, headers, body);

  // Send the generated response
  send(client_socket, response.c_str(), response.length(), 0);

  // Close connection
  close(client_socket);
}

// --- Main Server Loop (Unchanged) ---
int main()
{
  // Open DB
  if (sqlite3_open("users.db", &db))
  {
    std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
    return 1;
  }
  std::cout << "Database opened successfully." << std::endl;
  init_db(); // Initialize DB schema

  // Socket setup
  int server_fd;
  struct sockaddr_in address;
  int opt = 1;
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
  {
    perror("socket failed");
    sqlite3_close(db);
    exit(EXIT_FAILURE);
  }
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
  {
    perror("setsockopt failed");
    close(server_fd);
    sqlite3_close(db);
    exit(EXIT_FAILURE);
  }
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  // Bind
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
  {
    perror("bind failed");
    close(server_fd);
    sqlite3_close(db);
    exit(EXIT_FAILURE);
  }

  // Listen
  if (listen(server_fd, 10) < 0)
  { // Backlog of 10 connections
    perror("listen failed");
    close(server_fd);
    sqlite3_close(db);
    exit(EXIT_FAILURE);
  }

  std::cout << "Server listening on port " << PORT << std::endl;

  // Accept loop
  while (true)
  {
    int new_socket;
    int addrlen = sizeof(address);
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
      perror("accept failed");
      continue; // Don't stop server on one failed accept
    }
    // Handle client in a detached thread
    std::thread(handle_client, new_socket).detach();
  }

  // Cleanup (unreachable in current loop)
  std::cout << "Server shutting down." << std::endl;
  sqlite3_close(db);
  close(server_fd);
  return 0;
}
