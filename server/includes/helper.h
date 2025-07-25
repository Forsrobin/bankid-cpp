#include <iostream>

void logStartingServer()
{
  std::cout << "Starting BankID REST API server on port 8443 (HTTPS)..." << std::endl;
  std::cout << "Available endpoints:" << std::endl;
  std::cout << "  GET https://localhost:8443/init - Start authentication" << std::endl;
  std::cout << "  GET https://localhost:8443/poll - Check authentication status" << std::endl;
}