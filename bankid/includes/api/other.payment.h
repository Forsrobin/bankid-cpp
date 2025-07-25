#pragma once

#include "../bankid.h"
#include "payment.h" // For UserVisibleTransaction
#include "responses.h"
#include <string>
#include <optional>
#include <vector>

namespace BankID::API
{

  /**
   * Other Payment API configuration
   * Based on the /other/payment endpoint requirements
   */
  class BANKID_API OtherPaymentConfig
  {
  private:
    // Required fields
    std::string m_personalNumber;                    // Required: 12 digits
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
     * Constructor for OtherPaymentConfig
     * @param personalNumber The ID number of the user (required, 12 digits)
     * @param userVisibleTransaction Transaction information displayed to user (required)
     */
    OtherPaymentConfig(const std::string &personalNumber, const UserVisibleTransaction &userVisibleTransaction)
        : m_personalNumber(personalNumber), m_userVisibleTransaction(userVisibleTransaction) {}

    // Static factory methods
    static OtherPaymentConfig create(const std::string &personalNumber,
                                     const UserVisibleTransaction &transaction)
    {
      return OtherPaymentConfig(personalNumber, transaction);
    }

    static OtherPaymentConfig createApp(const std::string &personalNumber,
                                        const UserVisibleTransaction &transaction,
                                        const BankID::AppConfig &appConfig)
    {
      OtherPaymentConfig config(personalNumber, transaction);
      config.setAppConfig(appConfig);
      return config;
    }

    static OtherPaymentConfig createWeb(const std::string &personalNumber,
                                        const UserVisibleTransaction &transaction,
                                        const BankID::WebConfig &webConfig)
    {
      OtherPaymentConfig config(personalNumber, transaction);
      config.setWebConfig(webConfig);
      return config;
    }

    // Helper factory for card payments
    static OtherPaymentConfig createCardPayment(const std::string &personalNumber,
                                                const std::string &recipientName,
                                                const std::string &amount,
                                                const CurrencyCode &currency)
    {
      UserVisibleTransaction transaction;
      transaction.transactionType = "card";
      transaction.recipient.name = recipientName;
      transaction.money = PaymentMoney{amount, currency};
      return OtherPaymentConfig(personalNumber, transaction);
    }

    // Helper factory for NPA (Non-Payment Authentication)
    static OtherPaymentConfig createNPA(const std::string &personalNumber,
                                        const std::string &recipientName)
    {
      UserVisibleTransaction transaction;
      transaction.transactionType = "npa";
      transaction.recipient.name = recipientName;
      // money is not allowed for NPA type
      return OtherPaymentConfig(personalNumber, transaction);
    }

    // Getters
    const std::string &getPersonalNumber() const { return m_personalNumber; }
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
    OtherPaymentConfig &setReturnRisk(bool returnRisk)
    {
      m_returnRisk = returnRisk;
      return *this;
    }

    OtherPaymentConfig &setReturnUrl(const std::string &returnUrl)
    {
      m_returnUrl = returnUrl;
      return *this;
    }

    OtherPaymentConfig &setRiskFlags(const std::vector<std::string> &riskFlags)
    {
      m_riskFlags = riskFlags;
      return *this;
    }

    OtherPaymentConfig &setUserNonVisibleData(const std::string &data)
    {
      m_userNonVisibleData = data;
      return *this;
    }

    OtherPaymentConfig &setUserVisibleData(const std::string &data)
    {
      m_userVisibleData = data;
      return *this;
    }

    OtherPaymentConfig &setUserVisibleDataFormat(const std::string &format)
    {
      m_userVisibleDataFormat = format;
      return *this;
    }

    OtherPaymentConfig &setAppConfig(const BankID::AppConfig &appConfig)
    {
      m_appConfig = appConfig;
      return *this;
    }

    OtherPaymentConfig &setWebConfig(const BankID::WebConfig &webConfig)
    {
      m_webConfig = webConfig;
      return *this;
    }

    OtherPaymentConfig &setRequirement(const BankID::Requirement &requirement)
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
      j["personalNumber"] = m_personalNumber;

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
        if (req.pinCode.has_value())
        {
          reqJson["pinCode"] = req.pinCode.value();
        }
        // Note: personalNumber is handled separately in other payment, not in requirement

        if (!reqJson.empty())
        {
          j["requirement"] = reqJson;
        }
      }

      return j;
    }

    // Response type for other payment endpoint
    using ResponseType = LimitedResponse;
  };

} // namespace BankID::API
