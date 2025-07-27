#ifndef WEBSERVER_H
#define WEBSERVER_H

#pragma once

#include "bankid.h"
#include <crow.h>
#include <crow/middlewares/cors.h>

class WebServer {
public:
    WebServer();
    ~WebServer();
    
    bool initialize();
    void run(uint16_t port = 8080);
    void shutdown();
    
private:
    crow::App<crow::CORSHandler> m_app;
    std::unique_ptr<BankID::Session> m_bankidSession;
    BankID::API::AuthConfig m_authConfig;
    
    void setupCORS();
    void setupSSL();
    void setupRoutes();
    void printAvailableEndpoints() const;
    
    bool m_isInitialized = false;
};

#endif // WEBSERVER_H