#pragma once

#include "../bankid.h"
#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace BankID::API
{
  // Base response structure (for shared httpStatus field)
  struct BANKID_API DefaultResponse
  {
    int httpStatus;
  };

  inline void from_json(const nlohmann::json &j, DefaultResponse &r)
  {
    // You can optionally include shared response fields if they exist in JSON
    if (j.contains("httpStatus"))
      j.at("httpStatus").get_to(r.httpStatus);
  }

  /**
   * Order Response - used by /auth, /sign, /payment endpoints
   * Contains orderRef, autoStartToken, qrStartToken, qrStartSecret
   */
  struct BANKID_API OrderResponse : public DefaultResponse
  {
    std::string orderRef;
    std::string autoStartToken;
    std::string qrStartToken;
    std::string qrStartSecret;
  };

  inline void from_json(const nlohmann::json &j, OrderResponse &r)
  {
    from_json(j, static_cast<DefaultResponse &>(r));
    r.orderRef = j.at("orderRef").get<std::string>();
    r.autoStartToken = j.at("autoStartToken").get<std::string>();
    r.qrStartToken = j.at("qrStartToken").get<std::string>();
    r.qrStartSecret = j.at("qrStartSecret").get<std::string>();
  }

  /**
   * Limited Response - used by /phone/auth, /phone/sign, /other/payment endpoints
   * Contains only orderRef
   */
  struct BANKID_API LimitedResponse : public DefaultResponse
  {
    std::string orderRef;
  };

  inline void from_json(const nlohmann::json &j, LimitedResponse &r)
  {
    from_json(j, static_cast<DefaultResponse &>(r));
    r.orderRef = j.at("orderRef").get<std::string>();
  }

  /**
   * Empty Response - used by /cancel endpoint
   * Contains only the status field from base
   */
  struct BANKID_API EmptyResponse : public DefaultResponse
  {
    // No additional fields beyond the base
  };

  inline void from_json(const nlohmann::json &j, EmptyResponse &r)
  {
    from_json(j, static_cast<DefaultResponse &>(r));
  }

  /**
   * Error Response - used for error cases
   * Contains errorCode and details
   */
  struct BANKID_API ErrorResponse : public DefaultResponse
  {
    std::string errorCode;
    std::string details;
  };

  inline void from_json(const nlohmann::json &j, ErrorResponse &r)
  {
    from_json(j, static_cast<DefaultResponse &>(r));
    r.errorCode = j.at("errorCode").get<std::string>();
    r.details = j.at("details").get<std::string>();
  }

  /**
   * Collect Response structures for completion data
   */
  struct BANKID_API CollectResponseUser
  {
    std::optional<std::string> personalNumber;
    std::optional<std::string> name;
    std::optional<std::string> givenName;
    std::optional<std::string> surname;
  };

  inline void from_json(const nlohmann::json &j, CollectResponseUser &r)
  {
    if (j.contains("personalNumber"))
      r.personalNumber = j.at("personalNumber").get<std::string>();
    if (j.contains("name"))
      r.name = j.at("name").get<std::string>();
    if (j.contains("givenName"))
      r.givenName = j.at("givenName").get<std::string>();
    if (j.contains("surname"))
      r.surname = j.at("surname").get<std::string>();
  }

  struct BANKID_API CollectResponseDevice
  {
    std::optional<std::string> ipAddress;
    std::optional<std::string> uhi;
  };

  inline void from_json(const nlohmann::json &j, CollectResponseDevice &r)
  {
    if (j.contains("ipAddress"))
      r.ipAddress = j.at("ipAddress").get<std::string>();
    if (j.contains("uhi"))
      r.uhi = j.at("uhi").get<std::string>();
  }

  struct BANKID_API CollectResponseStepUp
  {
    std::optional<bool> mrtd;
  };

  inline void from_json(const nlohmann::json &j, CollectResponseStepUp &r)
  {
    if (j.contains("mrtd"))
      r.mrtd = j.at("mrtd").get<bool>();
  }

  /**
   * Risk values for collect endpoint
   */
  enum class CollectCompletionDataRisk
  {
    LOW,
    MODERATE,
    HIGH
  };

  struct BANKID_API CollectResponseCompletionData
  {
    std::optional<CollectResponseUser> user;
    std::optional<CollectResponseDevice> device;
    std::optional<CollectResponseStepUp> stepUp;
    std::optional<std::string> bankIdIssueDate;
    std::optional<std::string> signature;
    std::optional<std::string> ocspResponse;
    std::optional<CollectCompletionDataRisk> risk;
    std::string getRiskString() const
    {
      switch (risk.value_or(CollectCompletionDataRisk::LOW))
      {
      case CollectCompletionDataRisk::LOW:
        return "low";
      case CollectCompletionDataRisk::MODERATE:
        return "moderate";
      case CollectCompletionDataRisk::HIGH:
        return "high";
      default:
        return "unknown";
      }
    }
    CollectCompletionDataRisk fromString(const std::string &riskStr)
    {
      if (riskStr == "low")
        return CollectCompletionDataRisk::LOW;
      else if (riskStr == "moderate")
        return CollectCompletionDataRisk::MODERATE;
      else if (riskStr == "high")
        return CollectCompletionDataRisk::HIGH;
      else
        throw std::invalid_argument("Invalid collect risk string: " + riskStr);
    }
  };

  inline void from_json(const nlohmann::json &j, CollectResponseCompletionData &r)
  {
    if (j.contains("user"))
      r.user = j.at("user").get<CollectResponseUser>();
    if (j.contains("device"))
      r.device = j.at("device").get<CollectResponseDevice>();
    if (j.contains("stepUp"))
      r.stepUp = j.at("stepUp").get<CollectResponseStepUp>();
    if (j.contains("bankIdIssueDate"))
      r.bankIdIssueDate = j.at("bankIdIssueDate").get<std::string>();
    if (j.contains("signature"))
      r.signature = j.at("signature").get<std::string>();
    if (j.contains("ocspResponse"))
      r.ocspResponse = j.at("ocspResponse").get<std::string>();
    if (j.contains("risk"))
      r.risk = r.fromString(j.at("risk").get<std::string>());
  }

  /**
   * Status values for collect endpoint
   */
  enum class CollectStatus
  {
    PENDING,
    COMPLETE,
    FAILED
  };

  /**
   * Collect Response - used by /collect endpoint
   * Contains orderRef, status, completionData, hintCode
   */
  struct BANKID_API CollectResponse : public DefaultResponse
  {
    std::string orderRef;
    CollectStatus status;
    std::optional<CollectResponseCompletionData> completionData;
    std::optional<std::string> hintCode;
    std::string getStatusString() const
    {
      switch (status)
      {
      case CollectStatus::PENDING:
        return "pending";
      case CollectStatus::COMPLETE:
        return "complete";
      case CollectStatus::FAILED:
        return "failed";
      default:
        return "unknown";
      }
    }
    CollectStatus fromString(const std::string &statusStr)
    {
      if (statusStr == "pending")
        return CollectStatus::PENDING;
      else if (statusStr == "complete")
        return CollectStatus::COMPLETE;
      else if (statusStr == "failed")
        return CollectStatus::FAILED;
      else
        throw std::invalid_argument("Invalid collect status string: " + statusStr);
    }
  };

  inline void from_json(const nlohmann::json &j, CollectResponse &r)
  {
    from_json(j, static_cast<DefaultResponse &>(r));
    r.orderRef = j.at("orderRef").get<std::string>();
    r.status = r.fromString(j.at("status").get<std::string>());
    if (j.contains("completionData"))
      r.completionData = j.at("completionData").get<CollectResponseCompletionData>();
    if (j.contains("hintCode"))
      r.hintCode = j.at("hintCode").get<std::string>();
  }
}
