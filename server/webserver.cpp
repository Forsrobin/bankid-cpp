#include "webserver.h"
#include "routes.h"
#include <iostream>
#include <memory>

WebServer::WebServer() : m_authConfig("127.0.0.1")
{
  m_authConfig.setUserVisibleData("TG9nZ2luIHDDpSBjKysgdGVzdCBwb3J0YWxlbg==");
}

WebServer::~WebServer()
{
  shutdown();
}

bool WebServer::initialize()
{
  setupCORS();
  setupSSL();

  if (!m_bankidSession || !m_bankidSession->isInitialized())
  {
    std::cout << "BankID session initialization failed." << std::endl;
    return false;
  }

  setupRoutes();
  m_isInitialized = true;
  return true;
}

void WebServer::setupCORS()
{
  auto &cors = m_app.get_middleware<crow::CORSHandler>();

  cors
      .global()
      .headers("X-Custom-Header", "Upgrade-Insecure-Requests")
      .methods("POST"_method, "GET"_method)
      .prefix("/cors")
      .origin("localhost:3000")
      .prefix("/nocors")
      .ignore();
}

void WebServer::setupSSL()
{
  BankID::SSLConfig sslConfig(
      BankID::Environment::TEST,
      "certs/test.ca",
      "certs/bankid_cert.pem",
      "certs/bankid_key.pem");

  const auto &validationResult = sslConfig.validate();
  if (!validationResult.has_value())
  {
    std::cerr << "SSL configuration validation failed: " << validationResult.error() << std::endl;
    return;
  }

  m_bankidSession = std::make_unique<BankID::Session>(sslConfig);
}

void WebServer::setupRoutes()
{
  Routes::setupRoutes(m_app, *m_bankidSession, m_authConfig);
}

void WebServer::run(uint16_t port)
{
  if (!m_isInitialized)
  {
    std::cerr << "WebServer not initialized. Call initialize() first." << std::endl;
    return;
  }

  std::cout << "=== Server starting on port " << port << " ===" << std::endl;
  printAvailableEndpoints();
  std::cout << "Server is ready to accept connections!" << std::endl;
  std::cout << "=========================================" << std::endl;

  m_app.port(port).multithreaded().run();
}

void WebServer::printAvailableEndpoints() const
{
  const auto &endpoints = Routes::getEndpoints();

  if (endpoints.empty())
  {
    std::cout << "No endpoints registered." << std::endl;
    return;
  }

  std::cout << "\nAvailable endpoints:" << std::endl;

  // Find the maximum method and path lengths for formatting
  size_t maxMethodLen = 0;
  size_t maxPathLen = 0;

  for (const auto &endpoint : endpoints)
  {
    maxMethodLen = std::max(maxMethodLen, endpoint.method.length());
    maxPathLen = std::max(maxPathLen, endpoint.path.length());
  }

  // Print endpoints with aligned formatting
  for (const auto &endpoint : endpoints)
  {
    std::cout << "  "
              << std::left << std::setw(maxMethodLen) << endpoint.method
              << "  "
              << std::left << std::setw(maxPathLen) << endpoint.path
              << " - " << endpoint.description << std::endl;
  }
  std::cout << std::endl;
}

void WebServer::shutdown()
{
  BankID::QRGeneratorCache::instance().shutdown();
}