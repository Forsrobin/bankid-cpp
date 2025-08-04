#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "bankid.h"
#include "api/auth.h"
#include "api/responses.h"

class PhoneSignTest : public ::testing::Test
{
public:
  PhoneSignTest()
      : sslConfig(BankID::Environment::TEST),
        session(std::make_unique<BankID::Session>(sslConfig))
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

TEST_F(PhoneSignTest, PhoneSignUserInitiated)
{
  BankID::API::PhoneSignConfig config = BankID::API::PhoneSignConfig::createUserInitiated(BankID::Base64::encode("Test userVisibleData data"));
  config.setPersonalNumber("200003121145");

  EXPECT_EQ(config.getCallInitiator(), "user");
  EXPECT_TRUE(config.getPersonalNumber().has_value());
  EXPECT_FALSE(config.getUserNonVisibleData().has_value());
  EXPECT_EQ(config.getUserVisibleData(), BankID::Base64::encode("Test userVisibleData data"));
  EXPECT_FALSE(config.getUserVisibleDataFormat().has_value());
  EXPECT_FALSE(config.getRequirement().has_value());

  // Convert to JSON
  auto json = config.toJson();
  EXPECT_EQ(json["callInitiator"], "user");

  // Make the API call
  auto response = session->phoneSign(config);

  if (!response.has_value())
  {
    std::cerr << "Error: " << response.error().details << std::endl;
  }

  EXPECT_TRUE(response.has_value());
  const auto &limitedResponse = response.value();
  EXPECT_EQ(limitedResponse.httpStatus, 200);
  EXPECT_FALSE(limitedResponse.orderRef.empty());

  CancelOrder(limitedResponse.orderRef);
}

TEST_F(PhoneSignTest, PhoneSignRPInitiated)
{
  BankID::API::PhoneSignConfig config = BankID::API::PhoneSignConfig::createRPInitiated(BankID::Base64::encode("Test userVisibleData data"));
  config.setPersonalNumber("200003121145");

  EXPECT_EQ(config.getCallInitiator(), "RP");
  EXPECT_TRUE(config.getPersonalNumber().has_value());
  EXPECT_FALSE(config.getUserNonVisibleData().has_value());
  EXPECT_EQ(config.getUserVisibleData(), BankID::Base64::encode("Test userVisibleData data"));
  EXPECT_FALSE(config.getUserVisibleDataFormat().has_value());
  EXPECT_FALSE(config.getRequirement().has_value());

  // Convert to JSON
  auto json = config.toJson();
  EXPECT_EQ(json["callInitiator"], "RP");

  // Make the API call
  auto response = session->phoneSign(config);

  if (!response.has_value())
  {
    std::cerr << "Error: " << response.error().details << std::endl;
  }

  EXPECT_TRUE(response.has_value());
  const auto &limitedResponse = response.value();
  EXPECT_EQ(limitedResponse.httpStatus, 200);
  EXPECT_FALSE(limitedResponse.orderRef.empty());

  CancelOrder(limitedResponse.orderRef);
}

TEST_F(PhoneSignTest, PhoneSignenticationInvalidParameters)
{
  BankID::API::PhoneSignConfig config("random_initiator", "Invalid userVisibleData");
  config.setPersonalNumber("20000324");

  // Make the API call
  auto response = session->phoneSign(config);

  EXPECT_FALSE(response.has_value());
  if (!response.has_value())
  {
    EXPECT_EQ(response.error().details, R"({"errorCode":"invalidParameters","details":"Incorrect personalNumber"})");
    EXPECT_EQ(response.error().errorCode, BankID::BankIdErrorCode::INVALID_PARAMETERS);
    EXPECT_EQ(response.error().httpStatus, 400);
  }

  config.setPersonalNumber("200003121145");
  config.setUserVisibleDataFormat("simpleMarkdownV1");

  // Make the API call again with valid parameters
  response = session->phoneSign(config);

  EXPECT_FALSE(response.has_value());
  if (!response.has_value())
  {
    EXPECT_EQ(response.error().details, R"({"errorCode":"invalidParameters","details":"Incorrect callInitiator"})");
    EXPECT_EQ(response.error().errorCode, BankID::BankIdErrorCode::INVALID_PARAMETERS);
    EXPECT_EQ(response.error().httpStatus, 400);
  }

  config = BankID::API::PhoneSignConfig::createRPInitiated("Invalid userVisibleData data");
  config.setPersonalNumber("200003121145");

  // Make the API call again with valid parameters
  response = session->phoneSign(config);
  EXPECT_FALSE(response.has_value());
  if (!response.has_value())
  {
    EXPECT_EQ(response.error().details, R"({"errorCode":"invalidParameters","details":"Invalid userVisibleData"})");
    EXPECT_EQ(response.error().errorCode, BankID::BankIdErrorCode::INVALID_PARAMETERS);
    EXPECT_EQ(response.error().httpStatus, 400);  
  }
}

// Invalid parameters test
TEST_F(PhoneSignTest, PersonalNumberPhoneSignenticationAlreadyInProgress)
{

  BankID::API::PhoneSignConfig config = BankID::API::PhoneSignConfig::createWithPersonalNumber("user", BankID::Base64::encode("Test data"), "200003121145");

  // Make the API call
  auto response = session->phoneSign(config);

  EXPECT_TRUE(response.has_value());

  const auto &orderResponse = response.value();
  EXPECT_EQ(orderResponse.httpStatus, 200);
  EXPECT_FALSE(orderResponse.orderRef.empty());

  // Make the same API call again to trigger ALREADY_IN_PROGRESS error
  response = session->phoneSign(config);

  EXPECT_FALSE(response.has_value());

  if (!response.has_value())
  {
    EXPECT_EQ(response.error().details, R"({"errorCode":"alreadyInProgress","details":"Order already in progress for pno"})");
    EXPECT_EQ(response.error().errorCode, BankID::BankIdErrorCode::ALREADY_IN_PROGRESS);
    EXPECT_EQ(response.error().httpStatus, 400);
  }
}
