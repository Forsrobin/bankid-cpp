#pragma once

#include "../bankid.h"
#include "responses.h"
#include <string>
#include <optional>

namespace BankID::API
{

  /**
   * Authentication API configuration
   * Based on the /auth endpoint requirements
   */
  class BANKID_API AuthConfig
  {
  private:
    // Required fields
    std::string m_endUserIp;

    // Optional fields
    std::optional<bool> m_returnRisk;
    std::optional<std::string> m_returnUrl;
    std::optional<std::string> m_userNonVisibleData;    // base64-encoded, max 1500 chars
    std::optional<std::string> m_userVisibleData;       // base64-encoded UTF-8, max 1500 chars
    std::optional<std::string> m_userVisibleDataFormat; // "plaintext" or "simpleMarkdownV1"
    std::optional<BankID::AppConfig> m_appConfig;
    std::optional<BankID::WebConfig> m_webConfig;
    std::optional<BankID::Requirement> m_requirement;

  public:
    /**
     * Constructor for AuthConfig
     * @param endUserIp The IP address of the end user (required)
     */
    explicit AuthConfig(const std::string &endUserIp) : m_endUserIp(endUserIp) {}

    // Static factory methods
    static AuthConfig create(const std::string &endUserIp)
    {
      return AuthConfig(endUserIp);
    }

    static AuthConfig createApp(const std::string &endUserIp, const BankID::AppConfig &appConfig)
    {
      AuthConfig config(endUserIp);
      config.setAppConfig(appConfig);
      return config;
    }

    static AuthConfig createWeb(const std::string &endUserIp, const BankID::WebConfig &webConfig)
    {
      AuthConfig config(endUserIp);
      config.setWebConfig(webConfig);
      return config;
    }

    // Getters
    const std::string &getEndUserIp() const { return m_endUserIp; }
    const std::optional<bool> &getReturnRisk() const { return m_returnRisk; }
    const std::optional<std::string> &getReturnUrl() const { return m_returnUrl; }
    const std::optional<std::string> &getUserNonVisibleData() const { return m_userNonVisibleData; }
    const std::optional<std::string> &getUserVisibleData() const { return m_userVisibleData; }
    const std::optional<std::string> &getUserVisibleDataFormat() const { return m_userVisibleDataFormat; }
    const std::optional<BankID::AppConfig> &getAppConfig() const { return m_appConfig; }
    const std::optional<BankID::WebConfig> &getWebConfig() const { return m_webConfig; }
    const std::optional<BankID::Requirement> &getRequirement() const { return m_requirement; }

    // Fluent setters
    AuthConfig &setReturnRisk(bool returnRisk)
    {
      m_returnRisk = returnRisk;
      return *this;
    }

    AuthConfig &setReturnUrl(const std::string &returnUrl)
    {
      m_returnUrl = returnUrl;
      return *this;
    }

    AuthConfig &setUserNonVisibleData(const std::string &data)
    {
      m_userNonVisibleData = data;
      return *this;
    }

    AuthConfig &setUserVisibleData(const std::string &data)
    {
      m_userVisibleData = data;
      return *this;
    }

    AuthConfig &setUserVisibleDataFormat(const std::string &format)
    {
      m_userVisibleDataFormat = format;
      return *this;
    }

    AuthConfig &setAppConfig(const BankID::AppConfig &appConfig)
    {
      m_appConfig = appConfig;
      return *this;
    }

    AuthConfig &setWebConfig(const BankID::WebConfig &webConfig)
    {
      m_webConfig = webConfig;
      return *this;
    }

    AuthConfig &setRequirement(const BankID::Requirement &requirement)
    {
      m_requirement = requirement;
      return *this;
    }

    /**
     * Convert to JSON for API request
     */
    nlohmann::json toJson() const
    {
      nlohmann::json j;
      j["endUserIp"] = m_endUserIp;

      if (m_returnRisk.has_value())
      {
        j["returnRisk"] = m_returnRisk.value();
      }

      if (m_returnUrl.has_value())
      {
        j["returnUrl"] = m_returnUrl.value();
      }

      if (m_userNonVisibleData.has_value())
      {
        j["userNonVisibleData"] = m_userNonVisibleData.value();
      }

      if (m_userVisibleData.has_value())
      {
        j["userVisibleData"] = m_userVisibleData.value();
      }

      if (m_userVisibleDataFormat.has_value())
      {
        j["userVisibleDataFormat"] = m_userVisibleDataFormat.value();
      }

      if (m_appConfig.has_value())
      {
        const auto &app = m_appConfig.value();
        j["app"] = {
            {"appIdentifier", app.appIdentifier},
            {"deviceOS", app.deviceOS},
            {"deviceIdentifier", app.deviceIdentifier},
            {"deviceModelName", app.deviceModelName}};
      }

      if (m_webConfig.has_value())
      {
        const auto &web = m_webConfig.value();
        j["web"] = {
            {"deviceIdentifier", web.deviceIdentifier},
            {"referringDomain", web.referringDomain},
            {"userAgent", web.userAgent}};
      }

      if (m_requirement.has_value())
      {
        const auto &req = m_requirement.value();
        nlohmann::json reqJson;

        if (req.cardReader.has_value())
        {
          reqJson["cardReader"] = req.cardReader.value();
        }
        if (req.certificatePolicies.has_value())
        {
          reqJson["certificatePolicies"] = req.certificatePolicies.value();
        }
        if (req.mrtd.has_value())
        {
          reqJson["mrtd"] = req.mrtd.value();
        }
        if (req.personalNumber.has_value())
        {
          reqJson["personalNumber"] = req.personalNumber.value();
        }
        if (req.pinCode.has_value())
        {
          reqJson["pinCode"] = req.pinCode.value();
        }

        if (!reqJson.empty())
        {
          j["requirement"] = reqJson;
        }
      }

      return j;
    }

    // Response type for auth endpoint
    using ResponseType = OrderResponse;
  };

} // namespace BankID::API
