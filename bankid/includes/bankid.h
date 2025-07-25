#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include <string>
#include <optional>
#include <filesystem>
#include <vector>
#include <expected>
#include <iostream>
#include <chrono>
#include <thread>
#include <unordered_map>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#ifdef _WIN32
#ifdef BANKID_STATIC
#define BANKID_API
#elif defined(BANKID_EXPORTS)
#define BANKID_API __declspec(dllexport)
#else
#define BANKID_API __declspec(dllimport)
#endif
// Suppress C4251 warnings for STL types in DLL interface
#pragma warning(push)
#pragma warning(disable : 4251)
#else
#ifdef BANKID_EXPORTS
#define BANKID_API __attribute__((visibility("default")))
#else
#define BANKID_API
#endif
#endif

namespace BankID
{

  struct BANKID_API AppConfig
  {
    std::string appIdentifier;
    std::string deviceOS;
    std::string deviceIdentifier;
    std::string deviceModelName;
  };

  struct BANKID_API WebConfig
  {
    std::string deviceIdentifier;
    std::string referringDomain;
    std::string userAgent;
  };

  struct BANKID_API Requirement
  {
    std::optional<std::string> cardReader;
    std::optional<std::vector<std::string>> certificatePolicies;
    std::optional<bool> mrtd;
    std::optional<std::string> personalNumber;
    std::optional<bool> pinCode;
  };

  enum class BANKID_API Environment
  {
    TEST,
    PRODUCTION
  };

  enum class BANKID_API BankIdErrorCode
  {
    NOT_INITIALIZED,
    ALREADY_IN_PROGRESS,
    INVALID_PARAMETERS,
    UNAUTHORIZED,
    NOT_FOUND,
    METHOD_NOT_ALLOWED,
    REQUEST_TIMEOUT,
    UNSUPPORTED_MEDIA_TYPE,
    INTERNAL_ERROR,
    MAINTENANCE,
  };

  struct BANKID_API Base64
  {
    static std::string encode(const std::string &data)
    {
      std::string encoded = httplib::detail::base64_encode(data);
      return encoded;
    }
  };

  /** SSL configuration structure
   * This structure contains the SSL configuration for the BankID API.
   * It includes the environment, CA file path, PEM certificate path, and PEM key path.
   * It is used to configure the SSL client for secure communication with the BankID API.
   * To generate the PEM certificate and key, you can use OpenSSL to convert a PKCS#12 file (.p12) to PEM format.
   *
   * For example:
   *    openssl pkcs12 -in bankid_cert.p12 -out bankid_key.pem -nocerts -nodes
   *    openssl pkcs12 -in bankid_cert.p12 -out bankid_cert.pem -clcerts -nokeys
   * 
   * This structure is used to initialize the SSL client with the correct paths for the CA file, certificate, and key.
   */
  struct BANKID_API SSLConfig
  {
    Environment environment = Environment::TEST;
    std::string caFilePath = "certs/test.ca"; // Will be set based on environment
    std::string pemCertPath;                  // PEM certificate for Crow
    std::string pemKeyPath;                   // PEM private key for Crow

    /**
     * Constructor for SSLConfig
     * @param env Environment type (TEST or PRODUCTION)
     * @param caFilePath Path to the CA file
     * @param pemCertPath Path to the PEM certificate file
     * @param pemKeyPath Path to the PEM private key file
     * @brief Initializes the SSL configuration with the specified environment and certificate paths.
     * If the environment is TEST, it sets the CA file path to "certs/test.ca".
     * If the environment is PRODUCTION, it sets the CA file path to "certs/production.ca".
     *
     * The .ca file is used for server verification and are provided by BankID so no need to generate it.
     */
    SSLConfig(Environment env)
        : environment(env)
    {
      if (environment == Environment::TEST)
      {
        caFilePath = "certs/test.ca";
        pemCertPath = "certs/bankid_cert.pem";
        pemKeyPath = "certs/bankid_key.pem";
      }
      else
      {
        caFilePath = "certs/production.ca";
      }
    }

    SSLConfig(Environment env, const std::string &caFilePath, const std::string &pemCertPath, const std::string &pemKeyPath)
    {
      this->environment = env;
      this->caFilePath = caFilePath;
      this->pemCertPath = pemCertPath;
      this->pemKeyPath = pemKeyPath;
    };

    /**
     * Validate the SSL configuration
     * @return true if the configuration is valid, false otherwise
     * @brief Checks if the certificate and key files exist and are accessible.
     * If any of the files do not exist, it returns false and prints an error message.
     */
    std::expected<bool, std::string> validate() const
    {
      // Verify certificate files exist
      if (!std::filesystem::exists(this->pemCertPath))
      {
        return std::unexpected("Certificate file does not exist: " + this->pemCertPath);
      }

      if (!std::filesystem::exists(this->pemKeyPath))
      {
        return std::unexpected("Key file does not exist: " + this->pemKeyPath);
      }
      if (!std::filesystem::exists(this->caFilePath))
      {
        return std::unexpected("CA file does not exist: " + this->caFilePath);
      }

      return true;
    }
  };

}

// Include API endpoint configurations
#include "api/responses.h"
#include "api/auth.h"
#include "api/sign.h"
#include "api/payment.h"
#include "api/phone.auth.h"
#include "api/phone.sign.h"
#include "api/other.payment.h"
#include "api/collect.h"
#include "api/cancel.h"

namespace BankID
{
  // Now we can define AuthError after API namespace is available
  struct BANKID_API AuthError : public BankID::API::DefaultResponse
  {
    BankID::BankIdErrorCode errorCode;
    std::string details;
  };

  // BankID Session class for managing authentication sessions
  class BANKID_API Session
  {
  private:
    SSLConfig m_sslConfig;
    httplib::SSLClient *m_cli;
    bool m_initialized;
    const bool m_showDebugLog;

  public:
    Session(const SSLConfig &sslConfig, const bool showDebugLog = false);
    ~Session();

    /**
     * Start authentication process using AuthConfig
     * @param authConfig Configuration for the /auth endpoint
     * @return AuthResponse or AuthError
     */
    const std::expected<BankID::API::AuthConfig::ResponseType, BankID::AuthError> auth(const BankID::API::AuthConfig &authConfig);

    /**
     * Start signing process using SignConfig
     * @param signConfig Configuration for the /sign endpoint
     * @return AuthResponse or AuthError
     */
    const std::expected<BankID::API::SignConfig::ResponseType, BankID::AuthError> sign(const BankID::API::SignConfig &signConfig);

    /**
     * Start payment process using PaymentConfig
     * @param paymentConfig Configuration for the /payment endpoint
     * @return AuthResponse or AuthError
     */
    const std::expected<BankID::API::PaymentConfig::ResponseType, BankID::AuthError> payment(const BankID::API::PaymentConfig &paymentConfig);

    /**
     * Start phone authentication using PhoneAuthConfig
     * @param phoneAuthConfig Configuration for the /phone/auth endpoint
     * @return Limited response (orderRef only) or AuthError
     */
    const std::expected<BankID::API::PhoneAuthConfig::ResponseType, BankID::AuthError> phoneAuth(const BankID::API::PhoneAuthConfig &phoneAuthConfig);

    /**
     * Start phone signing using PhoneSignConfig
     * @param phoneSignConfig Configuration for the /phone/sign endpoint
     * @return Limited response (orderRef only) or AuthError
     */
    const std::expected<BankID::API::PhoneSignConfig::ResponseType, BankID::AuthError> phoneSign(const BankID::API::PhoneSignConfig &phoneSignConfig);

    /**
     * Start other payment using OtherPaymentConfig
     * @param otherPaymentConfig Configuration for the /other/payment endpoint
     * @return Limited response (orderRef only) or AuthError
     */
    const std::expected<BankID::API::OtherPaymentConfig::ResponseType, BankID::AuthError> otherPayment(const BankID::API::OtherPaymentConfig &otherPaymentConfig);

    /**
     * Collect order status and result
     * @param collectConfig Configuration for the /collect endpoint
     * @return Collect response or AuthError
     */
    const std::expected<BankID::API::CollectConfig::ResponseType, BankID::AuthError> collect(const BankID::API::CollectConfig &collectConfig);

    /**
     * Cancel an ongoing order
     * @param cancelConfig Configuration for the /cancel endpoint
     * @return Empty response or AuthError
     */
    const std::expected<BankID::API::CancelConfig::ResponseType, BankID::AuthError> cancel(const BankID::API::CancelConfig &cancelConfig);

    // Get current token
    bool isInitialized() const { return m_initialized; }
    bool initialize();
    const SSLConfig &getSSLConfig() const { return m_sslConfig; }

  private:
    /**
     * Generic method to make API calls
     * @param endpoint The API endpoint (e.g., "/auth", "/sign", etc.)
     * @param config Any config object that has a toJson() method
     * @return Expected response or error
     * @brief This method handles the actual HTTP request to the BankID API.
     * It constructs the request, sends it, and processes the response.
     * It uses the provided config object to generate the JSON payload.
     * It also validates the session state before making the request.
     */
    template <typename ConfigType>
    const std::expected<typename ConfigType::ResponseType, BankID::AuthError> makeApiCall(
        const std::string &endpoint,
        const ConfigType &config);

    /**
     * Validate response status and parse JSON
     * @param res The HTTP response from the API call
     * @param customErrors Optional map of custom error codes and messages
     * @return Parsed response or AuthError
     * @brief This method checks the HTTP status code and attempts to parse the response body as JSON.
     * If the status code is 200, it parses the JSON and returns the expected response type.
     * If the status code indicates an error, it checks for custom error mappings or attempts to parse an error response.
     * If parsing fails, it returns an AuthError with the appropriate error code and message.
     */
    template <typename T>
    const std::expected<T, BankID::AuthError> validateStatusAndParse(
        const httplib::Result &res,
        const std::unordered_map<int, std::string> &customErrors = {});
  };

  class BANKID_API QRGenerator
  {
  public:
    QRGenerator(std::string qrStartToken, std::string qrStartSecret);

    const std::expected<std::string, BankID::API::ErrorResponse> getNextQRCode(); // Returns next valid QR string
    bool isExpired() const;                                                       // Returns true if 30s has passed

  private:
    std::string m_qrStartToken;
    std::string m_qrStartSecret;
    std::chrono::steady_clock::time_point m_creationTime;

    int getElapsedSeconds() const;
    std::string computeAuthCode(int seconds) const;
  };

  /**
   * QRGeneratorCache class for caching QR code generators.
   * Using a singleton pattern to manage QR code generators.
   * Ensures only one shared cache exists throughout the application.
   *
   * This class provides methods to add, get, and remove QR code generators
   * based on order references. It also includes a cleanup loop to remove expired
   * generators periodically.
   */
  class BANKID_API QRGeneratorCache
  {
  public:
    static QRGeneratorCache &instance();

    void add(const std::string &orderRef, const std::string &qrStartToken, const std::string &qrStartSecret);
    std::shared_ptr<QRGenerator> get(const std::string &orderRef);
    void remove(const std::string &orderRef);

    void shutdown(); // Graceful stop

  private:
    QRGeneratorCache();
    ~QRGeneratorCache();

    void cleanupLoop();

    std::unordered_map<std::string, std::shared_ptr<QRGenerator>> m_cache;
    std::mutex m_cacheMutex;

    std::thread m_cleanerThread;
    std::condition_variable m_cv;
    std::mutex cv_mutex;
    std::atomic<bool> m_running;
  };

}

#ifdef _WIN32
#pragma warning(pop)
#endif
