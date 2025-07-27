#include "routes.h"
#include <iostream>

void Routes::setupRoutes(crow::App<crow::CORSHandler>& app, 
                        BankID::Session& bankidSession, 
                        BankID::API::AuthConfig& authConfig) {
    
    // Clear any existing endpoints
    endpoints.clear();
    
    // Register and setup GET /api/auth/init endpoint
    registerEndpoint("GET", "/api/auth/init", "Initialize BankID authentication");
    CROW_ROUTE(app, "/api/auth/init")
    ([&bankidSession, &authConfig]() {
        return handleAuthInit(bankidSession, authConfig);
    });

    // Register and setup GET /api/auth/poll endpoint
    registerEndpoint("GET", "/api/auth/poll/<orderRef>", "Poll authentication status");
    CROW_ROUTE(app, "/api/auth/poll/<string>")
    ([&bankidSession, &authConfig](std::string orderRef) {
        return handleAuthPoll(bankidSession, authConfig, orderRef);
    });

    // Register and setup GET /api/auth/cancel endpoint
    registerEndpoint("GET", "/api/auth/cancel/<orderRef>", "Cancel authentication request");
    CROW_ROUTE(app, "/api/auth/cancel/<string>")
    ([&bankidSession](std::string orderRef) {
        return handleAuthCancel(bankidSession, orderRef);
    });
}

const std::vector<EndpointInfo>& Routes::getEndpoints() {
    return endpoints;
}

void Routes::registerEndpoint(const std::string& method, 
                             const std::string& path, 
                             const std::string& description) {
    endpoints.push_back({method, path, description});
}

crow::response Routes::handleAuthInit(BankID::Session& bankidSession, 
                                     BankID::API::AuthConfig& authConfig) {
    std::cout << "GET /init - Starting authentication" << std::endl;
    
    auto response = bankidSession.auth(authConfig);

    if (!response) {
        const auto& error = response.error();
        return crow::response(error.httpStatus, error.details);
    }

    json res;
    res["orderRef"] = response->orderRef;
    res["autoStartToken"] = response->autoStartToken;
    res["authCountdown"] = 60;

    crow::response resp(200, res.dump());
    resp.add_header("Content-Type", "application/json");
    return resp;
}

crow::response Routes::handleAuthPoll(BankID::Session& bankidSession, 
                                     BankID::API::AuthConfig& authConfig, 
                                     const std::string& orderRef) {
    std::cout << "GET /api/auth/poll - Checking authentication status" << std::endl;
    std::cout << "Order Reference: " << orderRef << std::endl;

    auto collectConfig = BankID::API::CollectConfig::create(orderRef);
    auto response = bankidSession.collect(collectConfig);

    json baseResponse;
    baseResponse["status"] = response->getStatusString();
    baseResponse["orderRef"] = response->orderRef;
    baseResponse["qrCode"] = nullptr;
    baseResponse["token"] = nullptr;
    baseResponse["user"] = nullptr;

    if (!response) {
        const auto& error = response.error();
        return crow::response(error.httpStatus, error.details);
    }

    std::cout << "Collect response status: " << response->getStatusString() << std::endl;

    if (response->status == BankID::API::CollectStatus::PENDING) {
        auto qrCode = BankID::QRGeneratorCache::instance().get(response->orderRef)->getNextQRCode();

        if (!qrCode) {
            const auto& error = qrCode.error();
            return crow::response(error.httpStatus, error.details);
        }

        baseResponse["qrCode"] = qrCode.value();

        crow::response resp(200, baseResponse.dump());
        resp.add_header("Content-Type", "application/json");
        return resp;
    }
    else if (response->status == BankID::API::CollectStatus::COMPLETE) {
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
    else {
        auto newResponse = bankidSession.auth(authConfig);

        if (!newResponse) {
            const auto& error = response.error();
            return crow::response(error.httpStatus, error.details);
        }

        auto qrCode = BankID::QRGeneratorCache::instance().get(newResponse->orderRef);
        if (!qrCode) {
            return crow::response(404, "QR code not found in cache");
        }

        auto nextQrCode = qrCode->getNextQRCode();
        
        if (!nextQrCode) {
            const auto& error = nextQrCode.error();
            return crow::response(error.httpStatus, error.details);
        }

        baseResponse["qrCode"] = nextQrCode.value();
        baseResponse["orderRef"] = newResponse->orderRef;
        baseResponse["status"] = "pending";

        std::cout << "New orderRef: " << newResponse->orderRef << std::endl;
        std::cout << "sending back new payload: " << baseResponse.dump() << std::endl;

        crow::response resp(200, baseResponse.dump());
        resp.add_header("Content-Type", "application/json");
        return resp;
    }
}

crow::response Routes::handleAuthCancel(BankID::Session& bankidSession, 
                                       const std::string& orderRef) {
    auto cancelConfig = BankID::API::CancelConfig::create(orderRef);
    auto response = bankidSession.cancel(cancelConfig);

    if (!response) {
        const auto& error = response.error();
        return crow::response(error.httpStatus, error.details);
    }

    json res;
    res["message"] = "Order cancelled successfully";

    crow::response resp(200, res.dump());
    resp.add_header("Content-Type", "application/json");
    return resp;
}