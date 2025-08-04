#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "bankid.h"
#include "api/auth.h"
#include "api/responses.h"

class OtherPayment : public ::testing::Test
{
public:
  OtherPayment()
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

TEST_F(OtherPayment, ConfigOtherPaymentParameters)
{
  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "card";
  transaction.recipient = BankID::API::PaymentRecipient{"Test Recipient"};
  transaction.money = BankID::API::PaymentMoney{"100.00", BankID::API::CurrencyCode::SEK};

  BankID::API::OtherPaymentConfig config("200001010101", transaction);

  BankID::Requirement requirement;

  EXPECT_EQ(config.getPersonalNumber(), "200001010101");
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

TEST_F(OtherPayment, AppStartedOtherPayment)
{
  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "card";
  transaction.recipient = BankID::API::PaymentRecipient{"Test Recipient"};
  transaction.money = BankID::API::PaymentMoney{"100,00", BankID::API::CurrencyCode::SEK};

  BankID::API::OtherPaymentConfig config("200001010101", transaction);

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

  EXPECT_TRUE(config.getPersonalNumber() == "200001010101");

  EXPECT_FALSE(config.getReturnUrl().has_value());
  EXPECT_FALSE(config.getUserNonVisibleData().has_value());
  EXPECT_FALSE(config.getReturnRisk().has_value());

  // Set all the optional parameters
  config.setReturnUrl("https://example.com/return")
      .setUserNonVisibleData(BankID::Base64::encode("New OtherPayment text"))
      .setReturnRisk(true);

  EXPECT_TRUE(config.getReturnUrl().has_value());
  EXPECT_EQ(config.getReturnUrl().value(), "https://example.com/return");
  EXPECT_TRUE(config.getUserNonVisibleData().has_value());
  EXPECT_TRUE(config.getReturnRisk().has_value());
  EXPECT_EQ(config.getUserNonVisibleData().value(), BankID::Base64::encode("New OtherPayment text"));
  EXPECT_EQ(config.getReturnRisk().value(), true);

  // Make the API call
  auto response = session->otherPayment(config);

  if (!response.has_value())
  {
    FAIL() << "Sign request failed: " << response.error().details;
  }

  EXPECT_TRUE(response.has_value());

  // Check the response
  const auto &orderResponse = response.value();
  EXPECT_EQ(orderResponse.httpStatus, 200);
  EXPECT_FALSE(orderResponse.orderRef.empty());

  // Cancel the order
  CancelOrder(orderResponse.orderRef);
}

TEST_F(OtherPayment, WebStartedOtherPayment)
{
  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "card";
  transaction.recipient = BankID::API::PaymentRecipient{"Test Recipient"};
  transaction.money = BankID::API::PaymentMoney{"100,00", BankID::API::CurrencyCode::SEK};

  BankID::API::OtherPaymentConfig config("200001010101", transaction);

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
      .setUserNonVisibleData(BankID::Base64::encode("New OtherPayment text"))
      .setReturnRisk(false);

  EXPECT_TRUE(config.getReturnUrl().has_value());
  EXPECT_EQ(config.getReturnUrl().value(), "https://example.com/return");
  EXPECT_TRUE(config.getUserNonVisibleData().has_value());
  EXPECT_TRUE(config.getReturnRisk().has_value());
  EXPECT_EQ(config.getUserNonVisibleData().value(), BankID::Base64::encode("New OtherPayment text"));
  EXPECT_EQ(config.getReturnRisk().value(), false);

  // Make the API call
  auto response = session->otherPayment(config);

  EXPECT_TRUE(response.has_value());

  // Check the response
  const auto &orderResponse = response.value();
  EXPECT_EQ(orderResponse.httpStatus, 200);
  EXPECT_FALSE(orderResponse.orderRef.empty());


  CancelOrder(orderResponse.orderRef);
}

TEST_F(OtherPayment, SimpleOtherPayment)
{
  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "card";
  transaction.recipient = BankID::API::PaymentRecipient{"Test Recipient"};

  BankID::API::OtherPaymentConfig config("200001010101", transaction);

  EXPECT_TRUE(config.getReturnUrl().value_or("").empty());
  EXPECT_FALSE(config.getUserVisibleTransaction().transactionType.empty());
  EXPECT_FALSE(config.getUserVisibleTransaction().recipient.name.empty());
  EXPECT_FALSE(config.getUserVisibleTransaction().money.has_value());

  // Make the API call
  auto response = session->otherPayment(config);

  EXPECT_TRUE(response.has_value());

  // Check the response
  const auto &orderResponse = response.value();
  EXPECT_EQ(orderResponse.httpStatus, 200);
  EXPECT_FALSE(orderResponse.orderRef.empty());

  CancelOrder(orderResponse.orderRef);
}

TEST_F(OtherPayment, OtherPaymentInvalidPersonalNumber)
{
  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "card";
  transaction.recipient = BankID::API::PaymentRecipient{"Test Recipient"};

  BankID::API::OtherPaymentConfig config("aaaaaaaaaaaaaaaaa", transaction);

  EXPECT_TRUE(config.getReturnUrl().value_or("").empty());
  EXPECT_FALSE(config.getUserVisibleTransaction().transactionType.empty());
  EXPECT_FALSE(config.getUserVisibleTransaction().recipient.name.empty());

  // Make the API call
  auto response = session->otherPayment(config);

  EXPECT_FALSE(response.has_value());

  if (!response.has_value())
  {
    EXPECT_EQ(response.error().details, R"({"errorCode":"invalidParameters","details":"Incorrect personalNumber"})");
    EXPECT_EQ(response.error().errorCode, BankID::BankIdErrorCode::INVALID_PARAMETERS);
    EXPECT_EQ(response.error().httpStatus, 400);
  }
}

TEST_F(OtherPayment, OtherPaymentInvalidUserVisibleTransaction)
{

  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "random"; // Invalid transaction type
  transaction.recipient = BankID::API::PaymentRecipient{"Test Recipient"};

  BankID::API::OtherPaymentConfig config("200001010101", transaction);


  // Make the API call
  auto response = session->otherPayment(config);
  
  EXPECT_FALSE(response.has_value());
  
  if (!response.has_value())
  {
    EXPECT_EQ(response.error().details, R"({"errorCode":"invalidParameters","details":"transactionType is not a valid transaction type"})");
    EXPECT_EQ(response.error().errorCode, BankID::BankIdErrorCode::INVALID_PARAMETERS);
    EXPECT_EQ(response.error().httpStatus, 400);
  }
}

// Invalid parameters test
TEST_F(OtherPayment, PersonalNumberOtherPaymentAlreadyInProgress)
{
  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "card";
  transaction.recipient = BankID::API::PaymentRecipient{"Test Recipient"};

  BankID::API::OtherPaymentConfig config("200001010101", transaction);

  BankID::Requirement requirement;
  requirement.personalNumber = "200003121155";

  config.setRequirement(requirement);

  // Make the API call
  auto response = session->otherPayment(config);

  EXPECT_TRUE(response.has_value());

  const auto &orderResponse = response.value();
  EXPECT_EQ(orderResponse.httpStatus, 200);
  EXPECT_FALSE(orderResponse.orderRef.empty());


  // Make the same API call again to trigger ALREADY_IN_PROGRESS error
  response = session->otherPayment(config);

  EXPECT_FALSE(response.has_value());

  if (!response.has_value())
  {
    EXPECT_EQ(response.error().details, R"({"errorCode":"alreadyInProgress","details":"Order already in progress for pno"})");
    EXPECT_EQ(response.error().errorCode, BankID::BankIdErrorCode::ALREADY_IN_PROGRESS);
    EXPECT_EQ(response.error().httpStatus, 400);
  }
}

TEST_F(OtherPayment, OtherPaymentInvalidMoneyAmount)
{
  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "card";
  transaction.recipient = BankID::API::PaymentRecipient{"Test Recipient"};
  transaction.money = BankID::API::PaymentMoney{"100.00", BankID::API::CurrencyCode::SEK};

  BankID::API::OtherPaymentConfig config("200001010101", transaction);

  // Make the API call
  auto response = session->otherPayment(config);
  EXPECT_FALSE(response.has_value());

  if (!response.has_value())
  {
    EXPECT_EQ(response.error().details, R"({"errorCode":"invalidParameters","details":"money has invalid amount or currency"})");
    EXPECT_EQ(response.error().errorCode, BankID::BankIdErrorCode::INVALID_PARAMETERS);
    EXPECT_EQ(response.error().httpStatus, 400);
  }

}

TEST_F(OtherPayment, OtherPaymentInvalidCurrency)
{
  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "card";
  transaction.recipient = BankID::API::PaymentRecipient{"Test Recipient"};

  // Invalid currency code (simulate it by casting an invalid enum)
  transaction.money = BankID::API::PaymentMoney{"100.00", static_cast<BankID::API::CurrencyCode>(9999)};

  BankID::API::OtherPaymentConfig config("200001010101", transaction);

  auto response = session->otherPayment(config);
  EXPECT_FALSE(response.has_value());

  if (!response.has_value())
  {
    EXPECT_EQ(response.error().details, R"({"errorCode":"invalidParameters","details":"money has invalid amount or currency"})");
    EXPECT_EQ(response.error().errorCode, BankID::BankIdErrorCode::INVALID_PARAMETERS);
    EXPECT_EQ(response.error().httpStatus, 400);
  }
}


TEST_F(OtherPayment, OtherPaymentInvalidRecipient)
{
  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "card";

  // Invalid recipient (empty name)
  transaction.recipient = BankID::API::PaymentRecipient{""};
  transaction.money = BankID::API::PaymentMoney{"100,00", BankID::API::CurrencyCode::SEK};

  BankID::API::OtherPaymentConfig config("200001010101", transaction);

  auto response = session->otherPayment(config);
  EXPECT_FALSE(response.has_value());

  if (!response.has_value())
  {
    EXPECT_EQ(response.error().details, R"({"errorCode":"invalidParameters","details":"Recipient name is invalid"})");
    EXPECT_EQ(response.error().errorCode, BankID::BankIdErrorCode::INVALID_PARAMETERS);
    EXPECT_EQ(response.error().httpStatus, 400);
  }
}

TEST_F(OtherPayment, OtherPaymentInvalidUserNonVisibleData)
{
  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "card";
  transaction.recipient = BankID::API::PaymentRecipient{"Test Recipient"};
  transaction.money = BankID::API::PaymentMoney{"100,00", BankID::API::CurrencyCode::SEK};

  BankID::API::OtherPaymentConfig config("200001010101", transaction);

  // Set only userNonVisibleData without setting userVisibleData
  config.setUserNonVisibleData(BankID::Base64::encode("Hidden text"));

  auto response = session->otherPayment(config);
  EXPECT_FALSE(response.has_value());

  if (!response.has_value())
  {
    EXPECT_EQ(response.error().details, R"({"errorCode":"invalidParameters","details":"userNonVisible data requires userVisibleData"})");
    EXPECT_EQ(response.error().errorCode, BankID::BankIdErrorCode::INVALID_PARAMETERS);
    EXPECT_EQ(response.error().httpStatus, 400);
  }
}
