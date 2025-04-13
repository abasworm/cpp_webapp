#include "database.hpp" // Include the header file
#include <sqlite3.h>
#include <iostream>

// Static variable to hold the database connection within this file
static sqlite3 *db = nullptr;

// Function to initialize the database connection and create table
bool init_db(const std::string &db_path)
{
  if (sqlite3_open(db_path.c_str(), &db))
  {
    std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
    db = nullptr; // Ensure db is null on failure
    return false;
  }
  std::cout << "Database opened successfully at: " << db_path << std::endl;

  // Create table if not exists
  const char *sql = "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, username TEXT UNIQUE, password TEXT);";
  char *err = nullptr;
  if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK)
  {
    std::cerr << "Database error during init (CREATE TABLE): " << err << std::endl;
    sqlite3_free(err);
    sqlite3_close(db); // Close DB if table creation failed
    db = nullptr;
    return false;
  }
  return true;
}

// Function to close the database connection
void close_db()
{
  if (db)
  {
    sqlite3_close(db);
    db = nullptr;
    std::cout << "Database connection closed." << std::endl;
  }
}

// Function to get the database connection pointer
sqlite3 *get_db()
{
  return db;
}
