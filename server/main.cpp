#include "webserver.h"
#include <iostream>
#include <csignal>
#include <memory>

/**
 * @file main.cpp
 * @brief Main entry point for the BankID server application.
 *
 * This file initializes the web server and handles graceful shutdown.
 * It sets up signal handlers for cleanup on termination.
 * It also provides a usage message if no port is specified.
 
 * @note The server runs on the specified port and listens for incoming requests.
 * @note The server is designed to handle BankID authentication requests.
 * @note The application will clean up resources and shut down gracefully on receiving termination signals.
 * @note The server uses a singleton pattern for the QR code generator cache to manage QR codes
 *       and their expiration.
 */

std::unique_ptr<WebServer> server;

void handleShutdown(int signal)
{
  std::cout << "Received shutdown signal (" << signal << "). Cleaning up..." << std::endl;
  if (server)
  {
    server->shutdown();
  }
  std::_Exit(0);
}

int main(int argc, char *argv[])
{
  
  if (argc < 2)
  {
    std::cerr << "Usage: bankid_server.exe <port>" << std::endl;
    return 1;
  }

  const uint16_t port = static_cast<uint16_t>(std::stoi(argv[1]));

  // Create and initialize the web server
  server = std::make_unique<WebServer>();

  if (!server->initialize())
  {
    std::cerr << "Failed to initialize web server. Exiting." << std::endl;
    return 1;
  }

  // Register cleanup handlers
  std::atexit([]
              {
        std::cout << "Application is shutting down. Cleaning up..." << std::endl;
        if (server) {
            server->shutdown();
        } });

  std::signal(SIGINT, handleShutdown);  // Ctrl+C
  std::signal(SIGTERM, handleShutdown); // kill signal

  server->run(port);

  return 0;
}