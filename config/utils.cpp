#include "utils.hpp" // Include the header file
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <map>

// --- HTTP Response Helper Implementation ---

// Define the map (must match declaration in .hpp)
const std::map<int, std::string> http_status_reasons = {
    {200, "OK"}, {201, "Created"}, {400, "Bad Request"}, {404, "Not Found"}, {409, "Conflict"}, {415, "Unsupported Media Type"}, {500, "Internal Server Error"}};

std::string response_http(int status_code, const json &data, const std::string &content_type)
{
  std::string reason_phrase = "Unknown Status";
  auto it = http_status_reasons.find(status_code);
  if (it != http_status_reasons.end())
  {
    reason_phrase = it->second;
  }

  std::string body = data.dump(); // dump() can throw if data is invalid
  std::ostringstream oss;
  oss << "HTTP/1.1 " << status_code << " " << reason_phrase << "\r\n";
  oss << "Content-Type: " << content_type << "\r\n";
  oss << "Content-Length: " << body.length() << "\r\n";
  oss << "\r\n";
  oss << body;
  return oss.str();
}

// Overload for simple error messages
std::string response_http(int status_code, const std::string &error_message)
{
  json error_json = {{"error", error_message}};
  // Call the main response_http function
  return response_http(status_code, error_json);
}

// --- SHA256 Helper Implementation ---
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
