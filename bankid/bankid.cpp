#include "bankid.h"


namespace BankID
{
  // New Session implementation using SSLConfig only
  Session::Session(const SSLConfig &sslConfig, const bool showDebugLog)
      : m_sslConfig(sslConfig), m_cli(nullptr), m_initialized(false), m_showDebugLog(showDebugLog)
  {
    m_initialized = initialize();
  }

  Session::~Session()
  {
    if (m_cli)
    {
      delete m_cli;
    }
  }

  bool Session::initialize() 
  {
    if (!m_sslConfig.validate())
    {
      if (m_showDebugLog)
      {
        std::cerr << "BankID Session: SSL configuration validation failed" << std::endl;
      }
      m_cli = nullptr;
      return false;
    }

    this->m_cli = new httplib::SSLClient(m_sslConfig.environment == Environment::PRODUCTION ? "appapi2.bankid.com" : "appapi2.test.bankid.com",
                                         443,
                                         m_sslConfig.pemCertPath.c_str(),
                                         m_sslConfig.pemKeyPath.c_str());

    // Set CA certificate for server verification
    this->m_cli->set_ca_cert_path(m_sslConfig.caFilePath.c_str());
    this->m_cli->enable_server_certificate_verification(true);
    this->m_cli->enable_server_hostname_verification(m_sslConfig.environment == Environment::PRODUCTION);
    this->m_cli->set_connection_timeout(30);
    this->m_cli->set_read_timeout(30);

    return true;
  }

  // Endpoint-specific implementations
  const std::expected<BankID::API::AuthConfig::ResponseType, BankID::AuthError> Session::auth(const BankID::API::AuthConfig &authConfig)
  {
    auto response = makeApiCall("/auth", authConfig);

    if (response.has_value())
    {
      BankID::QRGeneratorCache::instance().add(response->orderRef, response->qrStartToken, response->qrStartSecret);
    }

    return response;
  }

  const std::expected<BankID::API::SignConfig::ResponseType, BankID::AuthError> Session::sign(const BankID::API::SignConfig &signConfig)
  {
    return makeApiCall("/sign", signConfig);
  }

  const std::expected<BankID::API::PaymentConfig::ResponseType, BankID::AuthError> Session::payment(const BankID::API::PaymentConfig &paymentConfig)
  {
    return makeApiCall("/payment", paymentConfig);
  }

  const std::expected<BankID::API::PhoneAuthConfig::ResponseType, BankID::AuthError> Session::phoneAuth(const BankID::API::PhoneAuthConfig &phoneAuthConfig)
  {
    return makeApiCall("/phone/auth", phoneAuthConfig);
  }

  const std::expected<BankID::API::PhoneSignConfig::ResponseType, BankID::AuthError> Session::phoneSign(const BankID::API::PhoneSignConfig &phoneSignConfig)
  {
    return makeApiCall("/phone/sign", phoneSignConfig);
  }

  const std::expected<BankID::API::OtherPaymentConfig::ResponseType, BankID::AuthError> Session::otherPayment(const BankID::API::OtherPaymentConfig &otherPaymentConfig)
  {
    return makeApiCall("/other/payment", otherPaymentConfig);
  }

  const std::expected<BankID::API::CollectConfig::ResponseType, BankID::AuthError> Session::collect(const BankID::API::CollectConfig &collectConfig)
  {
    return makeApiCall("/collect", collectConfig);
  }

  const std::expected<BankID::API::CancelConfig::ResponseType, BankID::AuthError> Session::cancel(const BankID::API::CancelConfig &cancelConfig)
  {
    // Clean up any QR code generator associated with this orderRef
    QRGeneratorCache::instance().remove(cancelConfig.getOrderRef());

    return makeApiCall("/cancel", cancelConfig);
  }

  // Generic template implementation for making API calls
  template <typename ConfigType>
  const std::expected<typename ConfigType::ResponseType, BankID::AuthError> Session::makeApiCall(
      const std::string &endpoint,
      const ConfigType &config)
  {
    if (!m_initialized)
    {
      if (m_showDebugLog)
      {
        std::cerr << "BankID Session: Session not initialized" << std::endl;
      }

      return std::unexpected(BankID::AuthError{
          {500},
          BankID::BankIdErrorCode::NOT_INITIALIZED, "Session not initialized"});
    }

    auto payload = config.toJson();

    if (m_showDebugLog)
    {
      std::cout << "BankID Session: Making API call to " << endpoint << std::endl;
      std::cout << "Payload: " << payload.dump(2) << std::endl;
    }

    payload = payload.dump();
    auto res = m_cli->Post(("/rp/v6.0" + endpoint).c_str(),
                           payload,
                           "application/json");

    return validateStatusAndParse<typename ConfigType::ResponseType>(res);
  }

  template <typename T>
  const std::expected<T, BankID::AuthError> Session::validateStatusAndParse(
      const httplib::Result &res,
      const std::unordered_map<int, std::string> &customErrors)
  {
    if (!res)
    {
      if (m_showDebugLog)
      {
        std::cerr << "BankID Session: No response from server" << std::endl;
        std::cerr << "Error: " << res.error() << std::endl;
      }

      return std::unexpected(AuthError{
          {403},
          BankIdErrorCode::INTERNAL_ERROR,
          "SSL server verification failed"});
    }

    // Success case
    if (res->status == 200)
    {
      try
      {
        nlohmann::json j = nlohmann::json::parse(res->body);
        T parsed;
        from_json(j, parsed);
        if constexpr (std::is_base_of_v<BankID::API::DefaultResponse, T>)
        {
          static_cast<BankID::API::DefaultResponse &>(parsed).httpStatus = res->status;
        }
        return parsed;
      }
      catch (const std::exception &e)
      {
        return std::unexpected(AuthError{
            {res->status},
            BankIdErrorCode::INVALID_PARAMETERS,
            std::string("Failed to parse response: ") + e.what()});
      }
    }

    // Custom error overrides
    auto it = customErrors.find(res->status);
    if (it != customErrors.end())
    {
      return std::unexpected(AuthError{
          {res->status},
          BankIdErrorCode::INVALID_PARAMETERS, // Or let caller map their own code
          it->second});
    }

    // Try to parse any error response as JSON first, fallback to default messages
    if (!res->body.empty())
    {
      try
      {
        // Validate that it's valid JSON
        auto jsonBody = nlohmann::json::parse(res->body);
        BankIdErrorCode errorCode;
        switch (res->status)
        {
        case 400:
          if (jsonBody.contains("errorCode"))
          {
            std::string errorCodeStr = jsonBody["errorCode"];
            if (errorCodeStr == "alreadyInProgress")
            {
              errorCode = BankIdErrorCode::ALREADY_IN_PROGRESS;
            }
            else if (errorCodeStr == "invalidParameters")
            {
              errorCode = BankIdErrorCode::INVALID_PARAMETERS;
            }
            else
            {
              errorCode = BankIdErrorCode::INTERNAL_ERROR;
            }
          }
          else
          {
            errorCode = BankIdErrorCode::INVALID_PARAMETERS;
          }
          break;
        case 401:
        case 403:
          errorCode = BankIdErrorCode::UNAUTHORIZED;
          break;
        case 404:
          errorCode = BankIdErrorCode::NOT_FOUND;
          break;
        case 405:
          errorCode = BankIdErrorCode::METHOD_NOT_ALLOWED;
          break;
        case 408:
          errorCode = BankIdErrorCode::REQUEST_TIMEOUT;
          break;
        case 415:
          errorCode = BankIdErrorCode::UNSUPPORTED_MEDIA_TYPE;
          break;
        case 500:
          errorCode = BankIdErrorCode::INTERNAL_ERROR;
          break;
        case 503:
          errorCode = BankIdErrorCode::MAINTENANCE;
          break;
        default:
          errorCode = BankIdErrorCode::INTERNAL_ERROR;
          break;
        }

        return std::unexpected(AuthError{
            {res->status},
            errorCode,
            res->body});
      }
      catch (const std::exception &e)
      {
        BankIdErrorCode errorCode;
        switch (res->status)
        {
        case 400:
          errorCode = BankIdErrorCode::INVALID_PARAMETERS;
          break;
        case 401:
        case 403:
          errorCode = BankIdErrorCode::UNAUTHORIZED;
          break;
        case 404:
          errorCode = BankIdErrorCode::NOT_FOUND;
          break;
        case 405:
          errorCode = BankIdErrorCode::METHOD_NOT_ALLOWED;
          break;
        case 408:
          errorCode = BankIdErrorCode::REQUEST_TIMEOUT;
          break;
        case 415:
          errorCode = BankIdErrorCode::UNSUPPORTED_MEDIA_TYPE;
          break;
        case 500:
          errorCode = BankIdErrorCode::INTERNAL_ERROR;
          break;
        case 503:
          errorCode = BankIdErrorCode::MAINTENANCE;
          break;
        default:
          errorCode = BankIdErrorCode::INTERNAL_ERROR;
          break;
        }

        return std::unexpected(AuthError{
            {res->status},
            errorCode,
            std::string("Non-JSON error response: ") + e.what() + " - " + res->body});
      }
    }

    // Status code → BankIdErrorCode and message mapping (fallback)
    const static std::unordered_map<int, std::pair<BankIdErrorCode, std::string>> defaultErrors = {
        {401, {BankIdErrorCode::UNAUTHORIZED, "You do not have access to the service."}},
        {403, {BankIdErrorCode::UNAUTHORIZED, "You do not have access to the service."}},
        {404, {BankIdErrorCode::NOT_FOUND, "An invalid URL path was used."}},
        {405, {BankIdErrorCode::METHOD_NOT_ALLOWED, "Only HTTP method POST is allowed."}},
        {408, {BankIdErrorCode::REQUEST_TIMEOUT, "Timeout while transmitting the request."}},
        {415, {BankIdErrorCode::UNSUPPORTED_MEDIA_TYPE, "The type is missing or invalid."}},
        {500, {BankIdErrorCode::INTERNAL_ERROR, "Internal technical error in the BankID system."}},
        {503, {BankIdErrorCode::MAINTENANCE, "The service is temporarily unavailable."}},
    };

    auto defIt = defaultErrors.find(res->status);
    if (defIt != defaultErrors.end())
    {
      return std::unexpected(AuthError{
          {res->status},
          defIt->second.first,
          defIt->second.second});
    }

    // Fallback for unknown codes
    return std::unexpected(AuthError{
        {res->status},
        BankIdErrorCode::INTERNAL_ERROR,
        "Unhandled HTTP error"});
  }

  // qr code generator cache implementation

  QRGenerator::QRGenerator(std::string token, std::string secret)
      : m_qrStartToken(std::move(token)), m_qrStartSecret(std::move(secret)),
        m_creationTime(std::chrono::steady_clock::now()) {}

  int QRGenerator::getElapsedSeconds() const
  {
    auto now = std::chrono::steady_clock::now();
    return static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(now - m_creationTime).count());
  }

  bool QRGenerator::isExpired() const
  {
    return getElapsedSeconds() >= 30;
  }

  std::string QRGenerator::computeAuthCode(int seconds) const
  {
    std::cout << "QRGenerator: Computing HMAC digest for seconds: " << seconds << std::endl;

    std::string timeStr = std::to_string(seconds);
    size_t keyLength = m_qrStartSecret.size();
    if (keyLength > static_cast<size_t>(INT_MAX))
    {
      throw std::runtime_error("Secret key length exceeds INT_MAX");
    }

    unsigned char *digest = HMAC(EVP_sha256(),
                                 reinterpret_cast<const unsigned char *>(m_qrStartSecret.data()),
                                 static_cast<int>(keyLength),
                                 reinterpret_cast<const unsigned char *>(timeStr.c_str()),
                                 static_cast<int>(timeStr.size()),
                                 nullptr, nullptr);

    if (!digest)
    {
      throw std::runtime_error("HMAC computation failed");
    }

    std::ostringstream oss;
    for (int i = 0; i < 32; ++i)
    {
      oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }

    return oss.str();
  }

  const std::expected<std::string, BankID::API::ErrorResponse> QRGenerator::getNextQRCode()
  {
    int seconds = getElapsedSeconds();
    if (isExpired())
      return std::unexpected(BankID::API::ErrorResponse{{404}, "QR code expired", "The QR code has expired after 30 seconds."});

    std::string authCode = computeAuthCode(seconds);
    return "bankid." + m_qrStartToken + "." + std::to_string(seconds) + "." + authCode;
  }

  QRGeneratorCache::QRGeneratorCache() : m_running(true)
  {
    m_cleanerThread = std::thread([this]()
                                 { cleanupLoop(); });
  }

  QRGeneratorCache::~QRGeneratorCache()
  {
    shutdown(); // Ensure graceful shutdown
  }

  QRGeneratorCache &QRGeneratorCache::instance()
  {
    static QRGeneratorCache instance;
    return instance;
  }

  void QRGeneratorCache::add(const std::string &orderRef, const std::string &qrStartToken, const std::string &qrStartSecret)
  {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_cache[orderRef] = std::make_shared<QRGenerator>(qrStartToken, qrStartSecret);
  }

  std::shared_ptr<QRGenerator> QRGeneratorCache::get(const std::string &orderRef)
  {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    auto it = m_cache.find(orderRef);
    if (it != m_cache.end())
    {
      if (it->second->isExpired())
      {
        m_cache.erase(it);
        return nullptr;
      }
      return it->second;
    }
    return nullptr;
  }

  void QRGeneratorCache::remove(const std::string &orderRef)
  {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_cache.erase(orderRef);
  }

  void QRGeneratorCache::shutdown()
  {
    {
      std::lock_guard<std::mutex> lock(cv_mutex);
      m_running = false;
    }
    m_cv.notify_all();
    if (m_cleanerThread.joinable())
    {
      m_cleanerThread.join();
    }
  }

  void QRGeneratorCache::cleanupLoop()
  {
    std::unique_lock<std::mutex> lock(cv_mutex);
    while (m_running)
    {
      m_cv.wait_for(lock, std::chrono::seconds(5), [this]()
                    { return !m_running; });

      if (!m_running)
        break;

      std::lock_guard<std::mutex> cacheLock(m_cacheMutex);
      for (auto it = m_cache.begin(); it != m_cache.end();)
      {
        if (it->second->isExpired())
        {
          it = m_cache.erase(it);
        }
        else
        {
          ++it;
        }
      }
    }
  }

}
