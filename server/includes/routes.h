#ifndef ROUTES_H
#define ROUTES_H

#pragma once

#include "bankid.h"
#include <crow.h>
#include <crow/middlewares/cors.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <string>

using json = nlohmann::json;

struct EndpointInfo
{
  std::string method;
  std::string path;
  std::string description;
};

class Routes
{
public:
  static void setupRoutes(crow::App<crow::CORSHandler> &app,
                          BankID::Session &bankidSession,
                          BankID::API::AuthConfig &authConfig);

  static const std::vector<EndpointInfo> &getEndpoints();

private:
  // Static member definition
  inline static std::vector<EndpointInfo> endpoints;

  static void registerEndpoint(const std::string &method,
                               const std::string &path,
                               const std::string &description);

  static crow::response handleAuthInit(BankID::Session &bankidSession,
                                       BankID::API::AuthConfig &authConfig);

  static crow::response handleAuthPoll(BankID::Session &bankidSession,
                                       BankID::API::AuthConfig &authConfig,
                                       const std::string &orderRef);

  static crow::response handleAuthCancel(BankID::Session &bankidSession,
                                         const std::string &orderRef);
};

#endif // ROUTES_H