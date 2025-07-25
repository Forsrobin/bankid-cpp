#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "bankid.h"
#include "api/auth.h"
#include "api/responses.h"

class SignTest : public ::testing::Test
{
public:
  SignTest()
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

TEST_F(SignTest, ConfigSigningParameters)
{
  BankID::API::SignConfig config("192.168.1.1", BankID::Base64::encode("This is a sample text to be signed"));

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

TEST_F(SignTest, AppStartedSigning)
{
  BankID::API::SignConfig config("127.0.0.1", BankID::Base64::encode("This is a sample text to be signed"));

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
      .setUserNonVisibleData(BankID::Base64::encode("New signing text"))
      .setReturnRisk(true);

  EXPECT_TRUE(config.getReturnUrl().has_value());
  EXPECT_EQ(config.getReturnUrl().value(), "https://example.com/return");
  EXPECT_TRUE(config.getUserNonVisibleData().has_value());
  EXPECT_TRUE(config.getReturnRisk().has_value());
  EXPECT_EQ(config.getUserNonVisibleData().value(), BankID::Base64::encode("New signing text"));
  EXPECT_EQ(config.getReturnRisk().value(), true);

  // Make the API call
  auto response = session->sign(config);

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

TEST_F(SignTest, WebStartedSigning)
{
  BankID::API::SignConfig config("127.0.0.1", BankID::Base64::encode("This is a sample text to be signed"));

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
      .setUserNonVisibleData(BankID::Base64::encode("New signing text"))
      .setReturnRisk(false);

  EXPECT_TRUE(config.getReturnUrl().has_value());
  EXPECT_EQ(config.getReturnUrl().value(), "https://example.com/return");
  EXPECT_TRUE(config.getUserNonVisibleData().has_value());
  EXPECT_TRUE(config.getReturnRisk().has_value());
  EXPECT_EQ(config.getUserNonVisibleData().value(), BankID::Base64::encode("New signing text"));
  EXPECT_EQ(config.getReturnRisk().value(), false);

  // Make the API call
  auto response = session->sign(config);

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

TEST_F(SignTest, PersonalNumberRequirementsSigning)
{
  BankID::API::SignConfig config("127.0.0.1", BankID::Base64::encode("This is a sample text to be signed"));

  // Requirements
  BankID::Requirement requirement;
  requirement.personalNumber = "200003121145";

  EXPECT_NO_THROW(config.setRequirement(requirement));
  EXPECT_TRUE(config.getRequirement().has_value());
  EXPECT_EQ(config.getRequirement()->personalNumber, "200003121145");
  EXPECT_FALSE(config.getRequirement()->cardReader.has_value());

  // Set all the optional parameters
  config.setReturnUrl("https://example.com/return")
      .setUserNonVisibleData(BankID::Base64::encode("New signing text"));

  EXPECT_TRUE(config.getReturnUrl().has_value());
  EXPECT_EQ(config.getReturnUrl().value(), "https://example.com/return");
  EXPECT_TRUE(config.getUserNonVisibleData().has_value());
  EXPECT_EQ(config.getUserNonVisibleData().value(), BankID::Base64::encode("New signing text"));

  // Make the API call
  auto response = session->sign(config);

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

TEST_F(SignTest, SimpleSigning)
{
  BankID::API::SignConfig config("127.0.0.1", BankID::Base64::encode("This is a sample text to be signed"));

  EXPECT_TRUE(config.getReturnUrl().value_or("").empty());
  EXPECT_FALSE(config.getUserVisibleData().empty());

  // Make the API call
  auto response = session->sign(config);

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

TEST_F(SignTest, SigningInvalidEnduserIp)
{
  BankID::API::SignConfig config("aaaaaaaaaaaaaaaaa", BankID::Base64::encode("This is a sample text to be signed"));

  EXPECT_TRUE(config.getReturnUrl().value_or("").empty());
  EXPECT_FALSE(config.getUserVisibleData().empty());

  // Make the API call
  auto response = session->sign(config);

  EXPECT_FALSE(response.has_value());

  if (!response.has_value())
  {
    EXPECT_EQ(response.error().details, R"({"errorCode":"invalidParameters","details":"Invalid endUserIp"})");
    EXPECT_EQ(response.error().errorCode, BankID::BankIdErrorCode::INVALID_PARAMETERS);
    EXPECT_EQ(response.error().httpStatus, 400);
  }
}

TEST_F(SignTest, SigningInvalidParameters)
{
  BankID::API::SignConfig config("127.0.0.1", BankID::Base64::encode("This is a sample text to be signed"));

  // Create requirement with invalid personal number
  BankID::Requirement requirement;
  requirement.personalNumber = "invalid_personal_number";

  config.setRequirement(requirement);

  // Make the API call
  auto response = session->sign(config);
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
  response = session->sign(config);
  EXPECT_FALSE(response.has_value());
  if (!response.has_value())
  {
    EXPECT_EQ(response.error().details, R"({"errorCode":"invalidParameters","details":"Invalid requirement"})");
    EXPECT_EQ(response.error().errorCode, BankID::BankIdErrorCode::INVALID_PARAMETERS);
    EXPECT_EQ(response.error().httpStatus, 400);
  }
}

// Invalid parameters test
TEST_F(SignTest, PersonalNumberSigningAlreadyInProgress)
{

  BankID::API::SignConfig config("127.0.0.1", BankID::Base64::encode("This is a sample text to be signed"));

  BankID::Requirement requirement;
  requirement.personalNumber = "200003121145";

  config.setRequirement(requirement);

  // Make the API call
  auto response = session->sign(config);

  EXPECT_TRUE(response.has_value());

  const auto &orderResponse = response.value();
  EXPECT_EQ(orderResponse.httpStatus, 200);
  EXPECT_FALSE(orderResponse.orderRef.empty());
  EXPECT_FALSE(orderResponse.autoStartToken.empty());
  EXPECT_FALSE(orderResponse.qrStartToken.empty());
  EXPECT_FALSE(orderResponse.qrStartSecret.empty());

  // Make the same API call again to trigger ALREADY_IN_PROGRESS error
  response = session->sign(config);

  EXPECT_FALSE(response.has_value());

  if (!response.has_value())
  {
    EXPECT_EQ(response.error().details, R"({"errorCode":"alreadyInProgress","details":"Order already in progress for pno"})");
    EXPECT_EQ(response.error().errorCode, BankID::BankIdErrorCode::ALREADY_IN_PROGRESS);
    EXPECT_EQ(response.error().httpStatus, 400);
  }
}
