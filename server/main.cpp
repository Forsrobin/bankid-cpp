#include "bankid.h"
#include "helper.h"
#include <crow.h>
#include <memory>
#include <string>
#include <csignal>
#include <crow/middlewares/cors.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

void handleShutdown(int signal)
{
  std::cout << "Received shutdown signal (" << signal << "). Cleaning up..." << std::endl;
  BankID::QRGeneratorCache::instance().shutdown();
  std::_Exit(0);
}

int main()
{
  // Enable CORS for the application
  crow::App<crow::CORSHandler> app;
  auto &cors = app.get_middleware<crow::CORSHandler>();

  // clang-format off
    cors
      .global()
        .headers("X-Custom-Header", "Upgrade-Insecure-Requests")
        .methods("POST"_method, "GET"_method)
      .prefix("/cors")
        .origin("localhost:3000")
      .prefix("/nocors")
        .ignore();


  // SSL Configuration for BankID
  BankID::SSLConfig sslConfig(
      BankID::Environment::TEST,
      "certs/bankid_cert.pem",
      "certs/bankid_key.pem");

  if (!sslConfig.validate())
  {
    return 1;
  }

  // Create your BankID session instance with just SSL config
  BankID::Session bankidSession(sslConfig);

  if (!bankidSession.isInitialized())
  {
    std::cout << "BankID session initialization failed. Exiting." << std::endl;
    return 1;
  }

  auto authConfig = BankID::API::AuthConfig("127.0.0.1").setUserVisibleData("TG9nZ2luIHDDpSBjKysgdGVzdCBwb3J0YWxlbg==");

  // GET /api/auth/init endpoint
  CROW_ROUTE(app, "/api/auth/init")
  ([&bankidSession, &authConfig]()
   {
        std::cout << "GET /init - Starting authentication" << std::endl;
        
        // Create auth config on-demand for this specific request


        auto response = bankidSession.auth(authConfig);

        if (!response)
        {
          const auto &error = response.error();
          return crow::response(error.httpStatus, error.details);
        }
        // Simple JSON response using nlohmann::json
        json res;
        res["orderRef"] = response->orderRef;
        res["autoStartToken"] = response->autoStartToken;
        res["authCountdown"] = 60;

        crow::response resp(200, res.dump());
        resp.add_header("Content-Type", "application/json");
        return resp; });

  // GET /api/auth/poll endpoint
  CROW_ROUTE(app, "/api/auth/poll/<string>")
  ([&bankidSession, &authConfig](std::string orderRef)
   {
      std::cout << "GET /api/auth/poll - Checking authentication status" << std::endl;
      std::cout << "Order Reference: " << orderRef << std::endl;

      // Create collect config on-demand for this specific request
      auto collectConfig = BankID::API::CollectConfig::create(orderRef);
      auto response = bankidSession.collect(collectConfig);

      json baseResponse;
      baseResponse["status"] = response->getStatusString();
      baseResponse["orderRef"] = response->orderRef;
      baseResponse["qrCode"] = nullptr; // Include QR code
      baseResponse["token"] = nullptr; // No token yet
      baseResponse["user"] = nullptr;  // No user data yet

      if (!response)
      {
        const auto &error = response.error();
        return crow::response(error.httpStatus, error.details);
      }

      std::cout << "Collect response status: " << response->getStatusString() << std::endl;

      if (response->status == BankID::API::CollectStatus::PENDING)
      {
        auto qrCode = BankID::QRGeneratorCache::instance().get(response->orderRef)->getNextQRCode();

        if (!qrCode)  
        {
          const auto &error = qrCode.error();
          return crow::response(error.httpStatus, error.details);
        }

        baseResponse["qrCode"] = qrCode.value(); // Include QR code

        crow::response resp(200, baseResponse.dump());
        resp.add_header("Content-Type", "application/json");
        return resp;
      }
      else if (response->status == BankID::API::CollectStatus::COMPLETE)
      {
        // Convert the response to JSON
      
        baseResponse["token"] = nullptr;
        baseResponse["user"] = response->completionData->user ? json{
          {"personalNumber", response->completionData->user->personalNumber},
          {"name", response->completionData->user->name},
          {"givenName", response->completionData->user->givenName},
          {"surname", response->completionData->user->surname}
        } : nullptr;  

        crow::response resp(200, baseResponse.dump());
        resp.add_header("Content-Type", "application/json");
        return resp;
      }
      else
      {
        // Try to get a QR code, if the QR code has expired, make a new request
        // to the /api/auth/init endpoint
        auto newResponse = bankidSession.auth(authConfig);


        if (!newResponse)
        {
          const auto &error = response.error();
          return crow::response(error.httpStatus, error.details);
        }

        auto qrCode = BankID::QRGeneratorCache::instance().get(newResponse->orderRef);
        if (!qrCode)
        {
          return crow::response(404, "QR code not found in cache");
        }

        auto nextQrCode = qrCode->getNextQRCode();
        
        if (!nextQrCode)
        {
          const auto &error = nextQrCode.error();
          return crow::response(error.httpStatus, error.details);
        }

        baseResponse["qrCode"] = nextQrCode.value(); // Include QR code
        baseResponse["orderRef"] = newResponse->orderRef;
        baseResponse["status"] = "pending";

        std::cout << "New orderRef: " << newResponse->orderRef << std::endl;
        std::cout << "sending back new payload: " << baseResponse.dump() << std::endl;

        crow::response resp(200, baseResponse.dump());
        resp.add_header("Content-Type", "application/json");
        return resp;
      } });

  CROW_ROUTE(app, "/api/auth/cancel/<string>")
  ([&bankidSession](std::string orderRef)
   {
        auto cancelConfig = BankID::API::CancelConfig::create(
            orderRef          // orderRef
        );

        auto response = bankidSession.cancel(cancelConfig);

        if (!response)
        {
          const auto &error = response.error();
          return crow::response(error.httpStatus, error.details);
        }


        // Convert the response to JSON
        json res;
        res["message"] = "Order cancelled successfully";

        crow::response resp(200, res.dump());
        resp.add_header("Content-Type", "application/json");
        return resp; });

  logStartingServer();

  // Register cleanup handler to ensure graceful shutdown
  std::atexit([]
              {
  std::cout << "Application is shutting down. Cleaning up QRGeneratorCache..." << std::endl;
  BankID::QRGeneratorCache::instance().shutdown(); });

  std::signal(SIGINT, handleShutdown);  // Ctrl+C
  std::signal(SIGTERM, handleShutdown); // kill signal

  app.port(8080).multithreaded().run();

  return 0;
}
