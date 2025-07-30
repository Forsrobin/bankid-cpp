#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "bankid.h"
#include "api/auth.h"
#include "api/responses.h"

class Payment : public ::testing::Test
{
public:
  Payment()
      : sslConfig(BankID::Environment::TEST),
        session(std::make_unique<BankID::Session>(sslConfig, false))
  {
  }

protected:
  void TearDown() override
  {
    session.reset();
  }

  void CancelOrder(std::string orderRef)
  {
    BankID::API::CancelConfig cancelConfig(orderRef);
    auto cancelResponse = session->cancel(cancelConfig);
    EXPECT_TRUE(cancelResponse.has_value());
    EXPECT_EQ(cancelResponse.value().httpStatus, 200);
  }

  BankID::SSLConfig sslConfig;
  std::unique_ptr<BankID::Session> session;
};

TEST_F(Payment, ConfigPaymentParameters)
{
  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "card";
  transaction.recipient = BankID::API::PaymentRecipient{"Test Recipient"};
  transaction.money = BankID::API::PaymentMoney{"100.00", BankID::API::CurrencyCode::SEK};

  BankID::API::PaymentConfig config("192.168.1.1", transaction);

  BankID::Requirement requirement;

  EXPECT_EQ(config.getEndUserIp(), "192.168.1.1");
  EXPECT_FALSE(config.getAppConfig().has_value());
  EXPECT_FALSE(config.getWebConfig().has_value());
  EXPECT_FALSE(config.getReturnUrl().has_value());

  // Create AppConfig
  BankID::AppConfig appConfig;
  appConfig.appIdentifier = "com.example.app";
  appConfig.deviceOS = "Android";
  appConfig.deviceIdentifier = "device123";

  // Set AppConfig
  config.setAppConfig(appConfig);
  EXPECT_TRUE(config.getAppConfig().has_value());

  EXPECT_FALSE(config.getAppConfig()->deviceModelName.length() > 0);

  appConfig.deviceModelName = "Pixel 5";
  EXPECT_NE(config.getAppConfig()->deviceModelName, "Pixel 5");
  EXPECT_EQ(appConfig.deviceModelName, "Pixel 5");

  // Create webConfig
  BankID::WebConfig webConfig;
  webConfig.deviceIdentifier = "webDevice123";
  webConfig.referringDomain = "example.com";
  webConfig.userAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.3";

  EXPECT_THROW(config.setWebConfig(webConfig), std::invalid_argument);
}

TEST_F(Payment, AppStartedPayment)
{
  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "card";
  transaction.recipient = BankID::API::PaymentRecipient{"Test Recipient"};
  transaction.money = BankID::API::PaymentMoney{"100,00", BankID::API::CurrencyCode::SEK};

  BankID::API::PaymentConfig config("127.0.0.1", transaction);

  config.setUserVisibleData(BankID::Base64::encode("This is a sample text to be signed"));

  // Create the appConfig
  BankID::AppConfig appConfig;
  appConfig.appIdentifier = "com.opsynk.com";
  appConfig.deviceOS = "IOS 14.4";
  appConfig.deviceModelName = "iPhone 12";
  appConfig.deviceIdentifier = "device123";

  config.setAppConfig(appConfig);

  EXPECT_TRUE(config.getAppConfig().has_value());
  EXPECT_TRUE(config.getAppConfig()->deviceModelName == "iPhone 12");

  EXPECT_TRUE(config.getEndUserIp() == "127.0.0.1");

  EXPECT_FALSE(config.getReturnUrl().has_value());
  EXPECT_FALSE(config.getUserNonVisibleData().has_value());
  EXPECT_FALSE(config.getReturnRisk().has_value());

  // Set all the optional parameters
  config.setReturnUrl("https://example.com/return")
      .setUserNonVisibleData(BankID::Base64::encode("New Payment text"))
      .setReturnRisk(true);

  EXPECT_TRUE(config.getReturnUrl().has_value());
  EXPECT_EQ(config.getReturnUrl().value(), "https://example.com/return");
  EXPECT_TRUE(config.getUserNonVisibleData().has_value());
  EXPECT_TRUE(config.getReturnRisk().has_value());
  EXPECT_EQ(config.getUserNonVisibleData().value(), BankID::Base64::encode("New Payment text"));
  EXPECT_EQ(config.getReturnRisk().value(), true);

  // Make the API call
  auto response = session->payment(config);

  if (!response.has_value())
  {
    FAIL() << "Sign request failed: " << response.error().details;
  }

  EXPECT_TRUE(response.has_value());

  // Check the response
  const auto &orderResponse = response.value();
  EXPECT_EQ(orderResponse.httpStatus, 200);
  EXPECT_FALSE(orderResponse.orderRef.empty());
  EXPECT_FALSE(orderResponse.autoStartToken.empty());
  EXPECT_FALSE(orderResponse.qrStartToken.empty());
  EXPECT_FALSE(orderResponse.qrStartSecret.empty());

  // Cancel the order
  CancelOrder(orderResponse.orderRef);
}

TEST_F(Payment, WebStartedPayment)
{
  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "card";
  transaction.recipient = BankID::API::PaymentRecipient{"Test Recipient"};
  transaction.money = BankID::API::PaymentMoney{"100,00", BankID::API::CurrencyCode::SEK};

  BankID::API::PaymentConfig config("127.0.0.1", transaction);

  // Create the webConfig
  BankID::WebConfig webConfig;
  webConfig.deviceIdentifier = "f1e3813ab36f114d4b0c2b3636617511467adb353ce8e5ae6c83500d932f2269";
  webConfig.referringDomain = "example.com";
  webConfig.userAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.3";

  EXPECT_NO_THROW(config.setWebConfig(webConfig));

  // Create the appConfig
  BankID::AppConfig appConfig;
  appConfig.appIdentifier = "com.opsynk.com";
  appConfig.deviceOS = "IOS 14.4";
  appConfig.deviceModelName = "iPhone 12";
  appConfig.deviceIdentifier = "device123";

  config.setUserVisibleData(BankID::Base64::encode("This is a sample text to be signed"));

  EXPECT_THROW(config.setAppConfig(appConfig), std::invalid_argument);
  EXPECT_FALSE(config.getAppConfig().has_value());

  EXPECT_TRUE(config.getWebConfig().has_value());
  EXPECT_TRUE(config.getWebConfig()->deviceIdentifier == "f1e3813ab36f114d4b0c2b3636617511467adb353ce8e5ae6c83500d932f2269");
  EXPECT_TRUE(config.getWebConfig()->referringDomain == "example.com");
  EXPECT_TRUE(config.getWebConfig()->userAgent == "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.3");

  EXPECT_FALSE(config.getReturnUrl().has_value());
  EXPECT_EQ(config.getUserVisibleData(), BankID::Base64::encode("This is a sample text to be signed"));
  EXPECT_FALSE(config.getReturnRisk().has_value());

  // Set all the optional parameters
  config.setReturnUrl("https://example.com/return")
      .setUserNonVisibleData(BankID::Base64::encode("New Payment text"))
      .setReturnRisk(false);

  EXPECT_TRUE(config.getReturnUrl().has_value());
  EXPECT_EQ(config.getReturnUrl().value(), "https://example.com/return");
  EXPECT_TRUE(config.getUserNonVisibleData().has_value());
  EXPECT_TRUE(config.getReturnRisk().has_value());
  EXPECT_EQ(config.getUserNonVisibleData().value(), BankID::Base64::encode("New Payment text"));
  EXPECT_EQ(config.getReturnRisk().value(), false);

  // Make the API call
  auto response = session->payment(config);

  EXPECT_TRUE(response.has_value());

  // Check the response
  const auto &orderResponse = response.value();
  EXPECT_EQ(orderResponse.httpStatus, 200);
  EXPECT_FALSE(orderResponse.orderRef.empty());
  EXPECT_FALSE(orderResponse.autoStartToken.empty());
  EXPECT_FALSE(orderResponse.qrStartToken.empty());
  EXPECT_FALSE(orderResponse.qrStartSecret.empty());

  CancelOrder(orderResponse.orderRef);
}

TEST_F(Payment, PersonalNumberRequirementsPayment)
{
  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "card";
  transaction.recipient = BankID::API::PaymentRecipient{"Test Recipient"};
  transaction.money = BankID::API::PaymentMoney{"100,00", BankID::API::CurrencyCode::SEK};

  BankID::API::PaymentConfig config("127.0.0.1", transaction);

  // Requirements
  BankID::Requirement requirement;
  requirement.personalNumber = "200003121145";

  EXPECT_NO_THROW(config.setRequirement(requirement));
  EXPECT_TRUE(config.getRequirement().has_value());
  EXPECT_EQ(config.getRequirement()->personalNumber, "200003121145");
  EXPECT_FALSE(config.getRequirement()->cardReader.has_value());

  // Set all the optional parameters
  config.setReturnUrl("https://example.com/return")
      .setUserVisibleData(BankID::Base64::encode("New Payment visible text"))
      .setUserNonVisibleData(BankID::Base64::encode("New Payment text"));

  EXPECT_TRUE(config.getReturnUrl().has_value());
  EXPECT_EQ(config.getReturnUrl().value(), "https://example.com/return");
  EXPECT_TRUE(config.getUserNonVisibleData().has_value());
  EXPECT_EQ(config.getUserNonVisibleData().value(), BankID::Base64::encode("New Payment text"));
  EXPECT_EQ(config.getUserVisibleData(), BankID::Base64::encode("New Payment visible text"));

  // Make the API call
  auto response = session->payment(config);

  if (!response.has_value())
  {
    std::cerr << "Payment request failed: " << response.error().details << std::endl;
    FAIL() << "Payment request failed: " << response.error().details;
  }

  EXPECT_TRUE(response.has_value());

  // Check the response
  const auto &orderResponse = response.value();
  EXPECT_EQ(orderResponse.httpStatus, 200);
  EXPECT_FALSE(orderResponse.orderRef.empty());
  EXPECT_FALSE(orderResponse.autoStartToken.empty());
  EXPECT_FALSE(orderResponse.qrStartToken.empty());
  EXPECT_FALSE(orderResponse.qrStartSecret.empty());

  CancelOrder(orderResponse.orderRef);
}

TEST_F(Payment, SimplePayment)
{
  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "card";
  transaction.recipient = BankID::API::PaymentRecipient{"Test Recipient"};

  BankID::API::PaymentConfig config("127.0.0.1", transaction);

  EXPECT_TRUE(config.getReturnUrl().value_or("").empty());
  EXPECT_FALSE(config.getUserVisibleTransaction().transactionType.empty());
  EXPECT_FALSE(config.getUserVisibleTransaction().recipient.name.empty());
  EXPECT_FALSE(config.getUserVisibleTransaction().money.has_value());

  // Make the API call
  auto response = session->payment(config);

  EXPECT_TRUE(response.has_value());

  // Check the response
  const auto &orderResponse = response.value();
  EXPECT_EQ(orderResponse.httpStatus, 200);
  EXPECT_FALSE(orderResponse.orderRef.empty());
  EXPECT_FALSE(orderResponse.autoStartToken.empty());
  EXPECT_FALSE(orderResponse.qrStartToken.empty());
  EXPECT_FALSE(orderResponse.qrStartSecret.empty());

  CancelOrder(orderResponse.orderRef);
}

TEST_F(Payment, PaymentInvalidEnduserIp)
{
  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "card";
  transaction.recipient = BankID::API::PaymentRecipient{"Test Recipient"};

  BankID::API::PaymentConfig config("aaaaaaaaaaaaaaaaa", transaction);

  EXPECT_TRUE(config.getReturnUrl().value_or("").empty());
  EXPECT_FALSE(config.getUserVisibleTransaction().transactionType.empty());
  EXPECT_FALSE(config.getUserVisibleTransaction().recipient.name.empty());

  // Make the API call
  auto response = session->payment(config);

  EXPECT_FALSE(response.has_value());

  if (!response.has_value())
  {
    EXPECT_EQ(response.error().details, R"({"errorCode":"invalidParameters","details":"Invalid endUserIp"})");
    EXPECT_EQ(response.error().errorCode, BankID::BankIdErrorCode::INVALID_PARAMETERS);
    EXPECT_EQ(response.error().httpStatus, 400);
  }
}

TEST_F(Payment, PaymentInvalidUserVisibleTransaction)
{

  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "random"; // Invalid transaction type
  transaction.recipient = BankID::API::PaymentRecipient{"Test Recipient"};

  BankID::API::PaymentConfig config("127.0.0.1", transaction);

  // Create requirement with invalid personal number
  BankID::Requirement requirement;
  requirement.personalNumber = "invalid_personal_number";
  config.setRequirement(requirement);

  // Make the API call
  auto response = session->payment(config);
  EXPECT_FALSE(response.has_value());
  if (!response.has_value())
  {
    EXPECT_EQ(response.error().details, R"({"errorCode":"invalidParameters","details":"Incorrect personalNumber"})");
    EXPECT_EQ(response.error().errorCode, BankID::BankIdErrorCode::INVALID_PARAMETERS);
    EXPECT_EQ(response.error().httpStatus, 400);
  }

  requirement.personalNumber = "200003121145"; // Valid personal number
  requirement.cardReader = "some_invalid_card_reader";
  config.setRequirement(requirement);

  // Make the API call
  response = session->payment(config);
  EXPECT_FALSE(response.has_value());

  if (!response.has_value())
  {
    EXPECT_EQ(response.error().details, R"({"errorCode":"invalidParameters","details":"transactionType is not a valid transaction type"})");
    EXPECT_EQ(response.error().errorCode, BankID::BankIdErrorCode::INVALID_PARAMETERS);
    EXPECT_EQ(response.error().httpStatus, 400);
  }
}

// Invalid parameters test
TEST_F(Payment, PersonalNumberPaymentAlreadyInProgress)
{
  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "card";
  transaction.recipient = BankID::API::PaymentRecipient{"Test Recipient"};

  BankID::API::PaymentConfig config("127.0.0.1", transaction);

  BankID::Requirement requirement;
  requirement.personalNumber = "200003121145";

  config.setRequirement(requirement);

  // Make the API call
  auto response = session->payment(config);

  EXPECT_TRUE(response.has_value());

  const auto &orderResponse = response.value();
  EXPECT_EQ(orderResponse.httpStatus, 200);
  EXPECT_FALSE(orderResponse.orderRef.empty());
  EXPECT_FALSE(orderResponse.autoStartToken.empty());
  EXPECT_FALSE(orderResponse.qrStartToken.empty());
  EXPECT_FALSE(orderResponse.qrStartSecret.empty());

  // Make the same API call again to trigger ALREADY_IN_PROGRESS error
  response = session->payment(config);

  EXPECT_FALSE(response.has_value());

  if (!response.has_value())
  {
    EXPECT_EQ(response.error().details, R"({"errorCode":"alreadyInProgress","details":"Order already in progress for pno"})");
    EXPECT_EQ(response.error().errorCode, BankID::BankIdErrorCode::ALREADY_IN_PROGRESS);
    EXPECT_EQ(response.error().httpStatus, 400);
  }
}

TEST_F(Payment, PaymentInvalidMoneyAmount)
{
  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "card";
  transaction.recipient = BankID::API::PaymentRecipient{"Test Recipient"};
  transaction.money = BankID::API::PaymentMoney{"100.00", BankID::API::CurrencyCode::SEK};

  BankID::API::PaymentConfig config("127.0.0.1", transaction);

  // Make the API call
  auto response = session->payment(config);
  EXPECT_FALSE(response.has_value());

  if (!response.has_value())
  {
    EXPECT_EQ(response.error().details, R"({"errorCode":"invalidParameters","details":"money has invalid amount or currency"})");
    EXPECT_EQ(response.error().errorCode, BankID::BankIdErrorCode::INVALID_PARAMETERS);
    EXPECT_EQ(response.error().httpStatus, 400);
  }

}

TEST_F(Payment, PaymentInvalidCurrency)
{
  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "card";
  transaction.recipient = BankID::API::PaymentRecipient{"Test Recipient"};

  // Invalid currency code (simulate it by casting an invalid enum)
  transaction.money = BankID::API::PaymentMoney{"100.00", static_cast<BankID::API::CurrencyCode>(9999)};

  BankID::API::PaymentConfig config("127.0.0.1", transaction);

  auto response = session->payment(config);
  EXPECT_FALSE(response.has_value());

  if (!response.has_value())
  {
    EXPECT_EQ(response.error().details, R"({"errorCode":"invalidParameters","details":"money has invalid amount or currency"})");
    EXPECT_EQ(response.error().errorCode, BankID::BankIdErrorCode::INVALID_PARAMETERS);
    EXPECT_EQ(response.error().httpStatus, 400);
  }
}


TEST_F(Payment, PaymentInvalidRecipient)
{
  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "card";

  // Invalid recipient (empty name)
  transaction.recipient = BankID::API::PaymentRecipient{""};
  transaction.money = BankID::API::PaymentMoney{"100,00", BankID::API::CurrencyCode::SEK};

  BankID::API::PaymentConfig config("127.0.0.1", transaction);

  auto response = session->payment(config);
  EXPECT_FALSE(response.has_value());

  if (!response.has_value())
  {
    EXPECT_EQ(response.error().details, R"({"errorCode":"invalidParameters","details":"Recipient name is invalid"})");
    EXPECT_EQ(response.error().errorCode, BankID::BankIdErrorCode::INVALID_PARAMETERS);
    EXPECT_EQ(response.error().httpStatus, 400);
  }
}

TEST_F(Payment, PaymentInvalidUserNonVisibleData)
{
  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "card";
  transaction.recipient = BankID::API::PaymentRecipient{"Test Recipient"};
  transaction.money = BankID::API::PaymentMoney{"100,00", BankID::API::CurrencyCode::SEK};

  BankID::API::PaymentConfig config("127.0.0.1", transaction);

  // Set only userNonVisibleData without setting userVisibleData
  config.setUserNonVisibleData(BankID::Base64::encode("Hidden text"));

  auto response = session->payment(config);
  EXPECT_FALSE(response.has_value());

  if (!response.has_value())
  {
    EXPECT_EQ(response.error().details, R"({"errorCode":"invalidParameters","details":"userNonVisible data requires userVisibleData"})");
    EXPECT_EQ(response.error().errorCode, BankID::BankIdErrorCode::INVALID_PARAMETERS);
    EXPECT_EQ(response.error().httpStatus, 400);
  }
}
