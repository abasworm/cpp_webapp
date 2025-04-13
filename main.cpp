#include <iostream>
#include <string>
#include <unordered_map>
#include <thread>
#include <sstream>
#include <vector>
#include <algorithm> // For std::transform
#include <cctype>    // For std::tolower
// Socket programming headers
#include <netinet/in.h>
#include <unistd.h>     // For read, close
#include <sys/socket.h> // For socket functions

// Include project headers
#include "config/database.hpp"
#include "config/utils.hpp"                   // For response_http on errors in handle_client
#include "app/controller/user_controller.hpp" // Include the user controller

// Define constants (consider moving to a config header/file later)
#define PORT 8080
#define READ_BUFFER_SIZE 4096

// --- Client Handling ---
// Handles reading the request, parsing basic HTTP info, and routing to controllers
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

  // Find end of headers (\r\n\r\n)
  size_t headers_end_pos = request_data.find("\r\n\r\n");
  if (headers_end_pos == std::string::npos)
  {
    std::cerr << "Could not find end of HTTP headers." << std::endl;
    std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 28\r\n\r\nMalformed request headers.";
    send(client_socket, response.c_str(), response.length(), 0);
    close(client_socket);
    return;
  }

  // Separate headers string and initial body part
  std::string headers_str = request_data.substr(0, headers_end_pos);
  std::string body_part = request_data.substr(headers_end_pos + 4); // Skip \r\n\r\n

  // Parse request line (Method Path Version)
  std::istringstream headers_stream(headers_str);
  std::string request_line;
  std::getline(headers_stream, request_line);
  if (!request_line.empty() && request_line.back() == '\r')
  {
    request_line.pop_back(); // Remove trailing '\r' if present
  }

  std::string method, path, version;
  std::istringstream request_line_stream(request_line);
  request_line_stream >> method >> path >> version;

  // Parse headers into a map
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
      break; // Blank line signifies end of headers

    size_t colon_pos = header_line.find(':');
    if (colon_pos != std::string::npos)
    {
      std::string name = header_line.substr(0, colon_pos);
      std::string value = header_line.substr(colon_pos + 1);
      // Trim whitespace from value
      value.erase(0, value.find_first_not_of(" \t"));
      value.erase(value.find_last_not_of(" \t") + 1);
      // Store header name lowercase
      std::transform(name.begin(), name.end(), name.begin(), ::tolower);
      headers[name] = value;
      // Check for content-length
      if (name == "content-length")
      {
        try
        {
          content_length = std::stoul(value);
        }
        catch (...)
        { // Catch potential exceptions from stoul
          std::cerr << "Invalid Content-Length value: " << value << std::endl;
          std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 29\r\n\r\nInvalid Content-Length header.";
          send(client_socket, response.c_str(), response.length(), 0);
          close(client_socket);
          return;
        }
      }
    }
  }

  // Read the rest of the body if necessary
  std::string body = body_part;
  if (content_length > 0 && body.length() < content_length)
  {
    size_t remaining = content_length - body.length();
    std::vector<char> extra_buffer(remaining);
    ssize_t extra_bytes_read = read(client_socket, extra_buffer.data(), remaining);
    if (extra_bytes_read > 0)
    {
      body.append(extra_buffer.data(), extra_bytes_read);
    }
    else
    {
      if (extra_bytes_read < 0)
        perror("read body failed");
      std::cerr << "Failed to read full request body." << std::endl;
      std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 26\r\n\r\nIncomplete request body.";
      send(client_socket, response.c_str(), response.length(), 0);
      close(client_socket);
      return;
    }
    // Final check if body length matches content-length
    if (body.length() != content_length)
    {
      std::cerr << "Body length mismatch after read." << std::endl;
      std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 26\r\n\r\nIncomplete request body.";
      send(client_socket, response.c_str(), response.length(), 0);
      close(client_socket);
      return;
    }
  }
  else if (body.length() > content_length)
  {
    body.resize(content_length); // Truncate if we read too much
  }

  // --- Routing to Controllers ---
  std::string response;
  // Basic routing: if path starts with /users, send to user controller
  if (path.rfind("/users", 0) == 0)
  { // Check if path starts with /users
    response = handle_user_request(method, path, headers, body);
  }
  else
  {
    // Handle other top-level routes here if needed
    response = response_http(404, json{{"message", "Endpoint not found"}}); // Default 404
  }

  // Send the response generated by the controller (or default 404)
  send(client_socket, response.c_str(), response.length(), 0);

  // Close the connection
  close(client_socket);
}

// --- Main Server Function ---
int main()
{
  // Initialize Database
  // Use database file in /root/ which is mapped to ./db_data by docker-compose
  if (!init_db("/root/users.db"))
  {
    return 1; // Exit if DB initialization fails
  }

  // Socket setup
  int server_fd;
  struct sockaddr_in address;
  int opt = 1;
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
  {
    perror("socket failed");
    close_db();
    exit(EXIT_FAILURE);
  }
  // Allow reuse of address and port
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
  {
    perror("setsockopt failed");
    close(server_fd);
    close_db();
    exit(EXIT_FAILURE);
  }
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY; // Listen on 0.0.0.0
  address.sin_port = htons(PORT);

  // Bind socket to port
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
  {
    perror("bind failed");
    close(server_fd);
    close_db();
    exit(EXIT_FAILURE);
  }

  // Listen for connections
  if (listen(server_fd, 10) < 0)
  { // Backlog of 10 connections
    perror("listen failed");
    close(server_fd);
    close_db();
    exit(EXIT_FAILURE);
  }

  std::cout << "Server listening on port " << PORT << std::endl;

  // Accept connections loop
  while (true)
  {
    int new_socket;
    struct sockaddr_in client_address;
    socklen_t addrlen = sizeof(client_address);
    if ((new_socket = accept(server_fd, (struct sockaddr *)&client_address, &addrlen)) < 0)
    {
      perror("accept failed");
      continue; // Continue listening even if one accept fails
    }

    // Handle client connection in a new detached thread
    // Note: Detached threads can be problematic for resource management on shutdown.
    // For a production server, a thread pool or joining threads on shutdown is better.
    std::thread(handle_client, new_socket).detach();
  }

  // --- Cleanup (Theoretically unreachable in this infinite loop) ---
  std::cout << "Server shutting down." << std::endl;
  close(server_fd); // Close listening socket
  close_db();       // Close database connection
  return 0;
}
