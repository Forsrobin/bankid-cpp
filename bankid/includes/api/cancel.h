#pragma once

#include "../bankid.h"
#include "responses.h"
#include <string>

namespace BankID::API
{

  /**
   * Cancel API configuration
   * Based on the /cancel endpoint requirements
   */
  class BANKID_API CancelConfig
  {
  private:
    std::string m_orderRef; // Required: Order reference to cancel

  public:
    /**
     * Constructor for CancelConfig
     * @param orderRef The order reference to cancel (required)
     */
    explicit CancelConfig(const std::string &orderRef) : m_orderRef(orderRef) {}

    // Static factory method
    static CancelConfig create(const std::string &orderRef)
    {
      return CancelConfig(orderRef);
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

    // Response type for cancel endpoint
    using ResponseType = EmptyResponse;
  };

} // namespace BankID::API
