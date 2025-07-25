#pragma once

#include "../bankid.h"
#include "responses.h"
#include <string>

namespace BankID::API
{

  /**
   * Collect API configuration
   * Based on the /collect endpoint requirements
   */
  class BANKID_API CollectConfig
  {
  private:
    std::string m_orderRef; // Required: Order reference to collect

  public:
    /**
     * Constructor for CollectConfig
     * @param orderRef The order reference to collect (required)
     */
    explicit CollectConfig(const std::string &orderRef) : m_orderRef(orderRef) {}

    // Static factory method
    static CollectConfig create(const std::string &orderRef)
    {
      return CollectConfig(orderRef);
    }

    // Getter
    const std::string &getOrderRef() const { return m_orderRef; }

    /**
     * Convert to JSON for API request
     */
    nlohmann::json toJson() const
    {
      nlohmann::json j;
      j["orderRef"] = m_orderRef;
      return j;
    }

    // Response type for collect endpoint
    using ResponseType = CollectResponse;
  };

} // namespace BankID::API
