#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# --- Compile Stage ---
# Compile each .cpp file into an object file (.o)
# -c flag means compile only, don't link
# -I flags specify include directories
echo "Compiling main.cpp..."
g++ -c main.cpp -o main.o -std=c++17 -I. -Iconfig -Iapp/controller -Iapp/model -Wall -Wextra

echo "Compiling database.cpp..."
g++ -c config/database.cpp -o database.o -std=c++17 -I. -Iconfig -Wall -Wextra

echo "Compiling utils.cpp..."
g++ -c config/utils.cpp -o utils.o -std=c++17 -I. -Iconfig -Wall -Wextra

echo "Compiling user_controller.cpp..."
g++ -c app/controller/user_controller.cpp -o user_controller.o -std=c++17 -I. -Iconfig -Iapp/controller -Iapp/model -Wall -Wextra

echo "Compiling user_model.cpp..."
g++ -c app/model/user_model.cpp -o user_model.o -std=c++17 -I. -Iconfig -Iapp/model -Wall -Wextra

# --- Link Stage ---
# Link all the object files together into the final executable 'app'
# -l flags link libraries (sqlite3, ssl, crypto, pthread)
# -static flag links libraries statically
echo "Linking object files into executable 'app'..."
g++ main.o database.o utils.o user_controller.o user_model.o \
    -o webserver \
    -lsqlite3 -lssl -lcrypto -lpthread -static

echo "Compilation and linking complete."
