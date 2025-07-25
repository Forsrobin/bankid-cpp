#pragma once

#include "../bankid.h"
#include "responses.h"
#include <string>
#include <optional>
#include <vector>

namespace BankID::API
{


  enum BANKID_API CurrencyCode
  {
    EUR, // Euro
    USD, // US Dollar
    SEK, // Swedish Krona
    NOK, // Norwegian Krone
    DKK, // Danish Krone
    GBP  // British Pound
  };
  
  inline std::string currencyToString(CurrencyCode code)
  {
    switch (code)
    {
    case EUR: return "EUR";
    case USD: return "USD";
    case SEK: return "SEK";
    case NOK: return "NOK";
    case DKK: return "DKK";
    case GBP: return "GBP";
    default: return "UNKNOWN";
    }
  }
  /**
   * User visible transaction structures for payment APIs
   */
  struct BANKID_API PaymentRecipient
  {
    std::string name; // Required, max 40 chars
  };

  struct BANKID_API PaymentMoney
  {
    std::string amount;   // Required, max 48 chars, e.g. "100,00"
    CurrencyCode currency; // Required, 3 chars, ISO 4217, e.g. "EUR"
  };

  struct BANKID_API UserVisibleTransaction
  {
    std::string transactionType;            // Required: "card" or "npa"
    PaymentRecipient recipient;             // Required
    std::optional<PaymentMoney> money;      // Optional, not allowed for "npa" type
    std::optional<std::string> riskWarning; // Optional: "newRecipient", "largeAmount", etc.
  };

  /**
   * Payment API configuration
   * Based on the /payment endpoint requirements
   */
  class BANKID_API PaymentConfig
  {
  private:
    // Required fields
    std::string m_endUserIp;
    UserVisibleTransaction m_userVisibleTransaction; // Required for payment endpoint

    // Optional fields
    std::optional<bool> m_returnRisk;
    std::optional<std::string> m_returnUrl;
    std::optional<std::vector<std::string>> m_riskFlags;
    std::optional<std::string> m_userNonVisibleData;    // base64-encoded, max 1000 chars
    std::optional<std::string> m_userVisibleData;       // base64-encoded UTF-8, max 1000 chars
    std::optional<std::string> m_userVisibleDataFormat; // "plaintext" or "simpleMarkdownV1"
    std::optional<BankID::AppConfig> m_appConfig;
    std::optional<BankID::WebConfig> m_webConfig;
    std::optional<BankID::Requirement> m_requirement;

  public:
    /**
     * Constructor for PaymentConfig
     * @param endUserIp The IP address of the end user (required)
     * @param userVisibleTransaction Transaction information displayed to user (required)
     */
    PaymentConfig(const std::string &endUserIp, const UserVisibleTransaction &userVisibleTransaction)
        : m_endUserIp(endUserIp), m_userVisibleTransaction(userVisibleTransaction) {}

    // Static factory methods
    static PaymentConfig create(const std::string &endUserIp, const UserVisibleTransaction &transaction)
    {
      return PaymentConfig(endUserIp, transaction);
    }

    static PaymentConfig createApp(const std::string &endUserIp, const UserVisibleTransaction &transaction,
                                   const BankID::AppConfig &appConfig)
    {
      PaymentConfig config(endUserIp, transaction);
      config.setAppConfig(appConfig);
      return config;
    }

    static PaymentConfig createWeb(const std::string &endUserIp, const UserVisibleTransaction &transaction,
                                   const BankID::WebConfig &webConfig)
    {
      PaymentConfig config(endUserIp, transaction);
      config.setWebConfig(webConfig);
      return config;
    }

    // Helper factory for card payments
    static PaymentConfig createCardPayment(const std::string &endUserIp, const std::string &recipientName,
                                           const std::string &amount, const CurrencyCode &currency)
    {
      UserVisibleTransaction transaction;
      transaction.transactionType = "card";
      transaction.recipient.name = recipientName;
      transaction.money = PaymentMoney{amount, currency};
      return PaymentConfig(endUserIp, transaction);
    }

    // Helper factory for NPA (Non-Payment Authentication)
    static PaymentConfig createNPA(const std::string &endUserIp, const std::string &recipientName)
    {
      UserVisibleTransaction transaction;
      transaction.transactionType = "npa";
      transaction.recipient.name = recipientName;
      // money is not allowed for NPA type
      return PaymentConfig(endUserIp, transaction);
    }

    // Getters
    const std::string &getEndUserIp() const { return m_endUserIp; }
    const UserVisibleTransaction &getUserVisibleTransaction() const { return m_userVisibleTransaction; }
    const std::optional<bool> &getReturnRisk() const { return m_returnRisk; }
    const std::optional<std::string> &getReturnUrl() const { return m_returnUrl; }
    const std::optional<std::vector<std::string>> &getRiskFlags() const { return m_riskFlags; }
    const std::optional<std::string> &getUserNonVisibleData() const { return m_userNonVisibleData; }
    const std::optional<std::string> &getUserVisibleData() const { return m_userVisibleData; }
    const std::optional<std::string> &getUserVisibleDataFormat() const { return m_userVisibleDataFormat; }
    const std::optional<BankID::AppConfig> &getAppConfig() const { return m_appConfig; }
    const std::optional<BankID::WebConfig> &getWebConfig() const { return m_webConfig; }
    const std::optional<BankID::Requirement> &getRequirement() const { return m_requirement; }

    // Fluent setters
    PaymentConfig &setReturnRisk(bool returnRisk)
    {
      m_returnRisk = returnRisk;
      return *this;
    }

    PaymentConfig &setReturnUrl(const std::string &returnUrl)
    {
      m_returnUrl = returnUrl;
      return *this;
    }

    PaymentConfig &setRiskFlags(const std::vector<std::string> &riskFlags)
    {
      m_riskFlags = riskFlags;
      return *this;
    }

    PaymentConfig &setUserNonVisibleData(const std::string &data)
    {
      m_userNonVisibleData = data;
      return *this;
    }

    PaymentConfig &setUserVisibleData(const std::string &data)
    {
      m_userVisibleData = data;
      return *this;
    }

    PaymentConfig &setUserVisibleDataFormat(const std::string &format)
    {
      m_userVisibleDataFormat = format;
      return *this;
    }

    PaymentConfig &setAppConfig(const BankID::AppConfig &appConfig)
    {
      m_appConfig = appConfig;
      return *this;
    }

    PaymentConfig &setWebConfig(const BankID::WebConfig &webConfig)
    {
      m_webConfig = webConfig;
      return *this;
    }

    PaymentConfig &setRequirement(const BankID::Requirement &requirement)
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

      // Required userVisibleTransaction
      nlohmann::json transactionJson;
      transactionJson["transactionType"] = m_userVisibleTransaction.transactionType;
      transactionJson["recipient"]["name"] = m_userVisibleTransaction.recipient.name;

      if (m_userVisibleTransaction.money.has_value())
      {
        const auto &money = m_userVisibleTransaction.money.value();
        transactionJson["money"]["amount"] = money.amount;
        transactionJson["money"]["currency"] = currencyToString(money.currency);
      }

      if (m_userVisibleTransaction.riskWarning.has_value())
      {
        transactionJson["riskWarning"] = m_userVisibleTransaction.riskWarning.value();
      }

      j["userVisibleTransaction"] = transactionJson;

      if (m_returnRisk.has_value())
      {
        j["returnRisk"] = m_returnRisk.value();
      }

      if (m_returnUrl.has_value())
      {
        j["returnUrl"] = m_returnUrl.value();
      }

      if (m_riskFlags.has_value())
      {
        j["riskFlags"] = m_riskFlags.value();
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

    // Response type for payment endpoint
    using ResponseType = OrderResponse;
  };

} // namespace BankID::API
