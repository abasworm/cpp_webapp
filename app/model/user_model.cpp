#include "user_model.hpp"         // Include the header file
#include "../config/database.hpp" // Access DB functions
#include "../config/utils.hpp"    // Access sha256 if needed directly (or pass hash)
#include <sqlite3.h>
#include <iostream>
#include <vector> // For potential future use

// Function to create a new user in the database
bool create_user_in_db(const std::string &username, const std::string &password, json &error_response)
{
  sqlite3 *db_conn = get_db(); // Get DB connection
  if (!db_conn)
  {
    std::cerr << "DB Error (create_user_in_db): No database connection." << std::endl;
    error_response = {{"error", "Database connection not available"}};
    return false; // Indicate failure
  }

  std::string hash = sha256(password); // Hash the password using utility function
  sqlite3_stmt *stmt;
  std::string sql = "INSERT INTO users (username, password) VALUES (?, ?);";

  if (sqlite3_prepare_v2(db_conn, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
  {
    std::cerr << "DB prepare error (create_user): " << sqlite3_errmsg(db_conn) << std::endl;
    error_response = {{"error", "Database preparation failed"}};
    return false;
  }

  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_STATIC);

  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (rc != SQLITE_DONE)
  {
    if (rc == SQLITE_CONSTRAINT)
    {
      error_response = {{"error", "Username already exists"}};
      // This is a client error (Conflict), but the DB operation itself didn't "fail" in a server sense
      // Depending on desired logic, you might return true but set error_response, or return false.
      // Let's return false to indicate the user wasn't created as requested.
      return false;
    }
    else
    {
      std::cerr << "DB execution error (create_user): " << sqlite3_errmsg(db_conn) << std::endl;
      error_response = {{"error", "Failed to create user in database"}};
      return false;
    }
  }
  // If successful, error_response remains empty/null
  return true; // Indicate success
}

// Function to retrieve all users from the database
json get_all_users_from_db(json &error_response)
{
  sqlite3 *db_conn = get_db();
  if (!db_conn)
  {
    std::cerr << "DB Error (get_all_users_from_db): No database connection." << std::endl;
    error_response = {{"error", "Database connection not available"}};
    return json::array(); // Return empty array on error
  }

  sqlite3_stmt *stmt;
  std::string sql = "SELECT id, username FROM users;";
  json users_json = json::array();

  if (sqlite3_prepare_v2(db_conn, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
  {
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
      json user;
      user["id"] = sqlite3_column_int(stmt, 0);
      const unsigned char *username_text = sqlite3_column_text(stmt, 1);
      user["username"] = username_text ? reinterpret_cast<const char *>(username_text) : "";
      users_json.push_back(user);
    }
    sqlite3_finalize(stmt);
  }
  else
  {
    std::cerr << "DB prepare error (get_users): " << sqlite3_errmsg(db_conn) << std::endl;
    error_response = {{"error", "Failed to retrieve users from database"}};
    return json::array(); // Return empty array on error
  }
  // If successful, error_response remains empty/null
  return users_json; // Return the array of users
}

// Function to delete a user from the database by ID
bool delete_user_from_db(int id, json &error_response)
{
  sqlite3 *db_conn = get_db();
  if (!db_conn)
  {
    std::cerr << "DB Error (delete_user_from_db): No database connection." << std::endl;
    error_response = {{"error", "Database connection not available"}};
    return false; // Indicate failure
  }

  sqlite3_stmt *stmt;
  std::string sql = "DELETE FROM users WHERE id = ?;";

  if (sqlite3_prepare_v2(db_conn, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
  {
    std::cerr << "DB prepare error (delete_user): " << sqlite3_errmsg(db_conn) << std::endl;
    error_response = {{"error", "Database preparation failed"}};
    return false;
  }

  sqlite3_bind_int(stmt, 1, id);
  int rc = sqlite3_step(stmt);
  int changes = sqlite3_changes(db_conn);
  sqlite3_finalize(stmt);

  if (rc != SQLITE_DONE)
  {
    std::cerr << "DB execution error (delete_user): " << sqlite3_errmsg(db_conn) << std::endl;
    error_response = {{"error", "Failed to delete user from database"}};
    return false;
  }

  if (changes == 0)
  {
    // User ID not found is a client error, not a server failure
    error_response = {{"error", "User not found"}};
    // Return false as the deletion didn't happen as requested
    return false;
  }
  // If successful, error_response remains empty/null
  return true; // Indicate success
}
