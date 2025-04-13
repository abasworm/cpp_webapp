# Dockerfile

# ---- Build Stage ----
# Use a specific version of the GCC compiler image as the builder
FROM gcc:13.3.0 as builder

# Install necessary development libraries for SQLite3 and OpenSSL
RUN apt-get update && apt-get install -y --no-install-recommends \
  libsqlite3-dev \
  libssl-dev \
  && rm -rf /var/lib/apt/lists/*

# Set the working directory inside the container
WORKDIR /usr/src/app

# Copy the JSON header file first (needed for compilation)
COPY json.hpp .

# Copy the C++ source file into the container
COPY main.cpp .

# Compile the C++ application
# Link against SQLite3, SSL, Crypto, and Pthread libraries
# Use -static for potentially smaller image, but might have issues on Alpine
# Consider removing -static if runtime issues occur on Alpine
RUN g++ main.cpp -o app -std=c++17 -Wall -Wextra -lsqlite3 -lssl -lcrypto -lpthread -static

# # ---- Final Stage ----
# # Use a minimal Alpine base image for the final container
# FROM alpine:latest

# # Install necessary runtime libraries
# RUN apk add --no-cache sqlite-libs openssl-libs-static

# # Set the working directory for the final image
# WORKDIR /root/

# # Copy the compiled application from the builder stage
# COPY --from=builder /usr/src/app /root/

# # Copy the database file (if you want to bundle an initial empty one)
# # RUN touch users.db # Optional: create an empty db file if needed on first run

# # Expose the port the application listens on
EXPOSE 8080

# Command to run the application when the container starts
# The database file 'users.db' will be created in the WORKDIR ('/root/') if it doesn't exist
CMD ["./app"]
