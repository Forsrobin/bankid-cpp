#pragma once

#include "../bankid.h"
#include "responses.h"
#include <string>
#include <optional>

namespace BankID::API
{

  /**
   * Phone Sign API configuration
   * Based on the /phone/sign endpoint requirements
   */
  class BANKID_API PhoneSignConfig
  {
  private:
    // Required fields
    std::string m_callInitiator;   // Required: "user" or "RP"
    std::string m_userVisibleData; // Required for sign endpoint

    // Optional fields
    std::optional<std::string> m_personalNumber;        // 12 digits
    std::optional<std::string> m_userNonVisibleData;    // base64-encoded, max 200000 chars
    std::optional<std::string> m_userVisibleDataFormat; // "plaintext" or "simpleMarkdownV1"
    std::optional<BankID::Requirement> m_requirement;

  public:
    /**
     * Constructor for PhoneSignConfig
     * @param callInitiator Indicates who initiated the call: "user" or "RP" (required)
     * @param userVisibleData Text displayed to the user during signing (required)
     */
    PhoneSignConfig(const std::string &callInitiator, const std::string &userVisibleData)
        : m_callInitiator(callInitiator), m_userVisibleData(userVisibleData) {}

    // Static factory methods
    static PhoneSignConfig createUserInitiated(const std::string &userVisibleData)
    {
      return PhoneSignConfig("user", userVisibleData);
    }

    static PhoneSignConfig createRPInitiated(const std::string &userVisibleData)
    {
      return PhoneSignConfig("RP", userVisibleData);
    }

    static PhoneSignConfig createWithPersonalNumber(const std::string &callInitiator,
                                                    const std::string &userVisibleData,
                                                    const std::string &personalNumber)
    {
      PhoneSignConfig config(callInitiator, userVisibleData);
      config.setPersonalNumber(personalNumber);
      return config;
    }

    // Getters
    const std::string &getCallInitiator() const { return m_callInitiator; }
    const std::string &getUserVisibleData() const { return m_userVisibleData; }
    const std::optional<std::string> &getPersonalNumber() const { return m_personalNumber; }
    const std::optional<std::string> &getUserNonVisibleData() const { return m_userNonVisibleData; }
    const std::optional<std::string> &getUserVisibleDataFormat() const { return m_userVisibleDataFormat; }
    const std::optional<BankID::Requirement> &getRequirement() const { return m_requirement; }

    // Fluent setters
    PhoneSignConfig &setPersonalNumber(const std::string &personalNumber)
    {
      m_personalNumber = personalNumber;
      return *this;
    }

    PhoneSignConfig &setUserNonVisibleData(const std::string &data)
    {
      m_userNonVisibleData = data;
      return *this;
    }

    PhoneSignConfig &setUserVisibleDataFormat(const std::string &format)
    {
      m_userVisibleDataFormat = format;
      return *this;
    }

    PhoneSignConfig &setRequirement(const BankID::Requirement &requirement)
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
      j["callInitiator"] = m_callInitiator;
      j["userVisibleData"] = m_userVisibleData; // Required field

      if (m_personalNumber.has_value())
      {
        j["personalNumber"] = m_personalNumber.value();
      }

      if (m_userNonVisibleData.has_value())
      {
        j["userNonVisibleData"] = m_userNonVisibleData.value();
      }

      if (m_userVisibleDataFormat.has_value())
      {
        j["userVisibleDataFormat"] = m_userVisibleDataFormat.value();
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
        // Note: personalNumber is not part of requirement for phone sign

        if (!reqJson.empty())
        {
          j["requirement"] = reqJson;
        }
      }

      return j;
    }

    // Response type for phone sign endpoint
    using ResponseType = LimitedResponse;
  };

} // namespace BankID::API
