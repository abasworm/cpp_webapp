#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <sqlite3.h>
#include <string>

// Function declarations
bool init_db(const std::string &db_path = "users.db"); // Returns true on success, false on failure
void close_db();
sqlite3 *get_db(); // Function to get the database connection pointer

#endif // DATABASE_HPP
