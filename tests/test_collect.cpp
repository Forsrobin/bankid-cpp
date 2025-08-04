#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "bankid.h"
#include "api/auth.h"
#include "api/responses.h"

class CollectTest : public ::testing::Test
{
public:
  CollectTest()
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

TEST_F(CollectTest, CollectAuth)
{
  BankID::API::AuthConfig config("127.0.0.1");

  EXPECT_TRUE(config.getReturnUrl().value_or("").empty());
  EXPECT_TRUE(config.getUserVisibleData().value_or("").empty());

  // Make the API call
  auto response = session->auth(config);

  EXPECT_TRUE(response.has_value());

  // Check the response
  const auto &orderResponse = response.value();
  EXPECT_EQ(orderResponse.httpStatus, 200);
  EXPECT_FALSE(orderResponse.orderRef.empty());

  BankID::API::CollectConfig collectConfig(orderResponse.orderRef);

  auto collectResponse = session->collect(collectConfig);
  EXPECT_TRUE(collectResponse.has_value());
  EXPECT_EQ(collectResponse.value().httpStatus, 200);
  EXPECT_EQ(collectResponse.value().orderRef, orderResponse.orderRef);
  EXPECT_EQ(collectResponse.value().status, BankID::API::CollectStatus::PENDING);

  BankID::API::CancelConfig cancelConfig(orderResponse.orderRef);
  auto cancelResponse = session->cancel(cancelConfig);
  EXPECT_TRUE(cancelResponse.has_value());
  EXPECT_EQ(cancelResponse.value().httpStatus, 200);

  collectResponse = session->collect(collectConfig);

  EXPECT_FALSE(collectResponse.has_value());

  if (!collectResponse.has_value())
  {
    EXPECT_EQ(collectResponse.error().details, R"({"errorCode":"invalidParameters","details":"No such order"})");
    EXPECT_EQ(collectResponse.error().errorCode, BankID::BankIdErrorCode::INVALID_PARAMETERS);
    EXPECT_EQ(collectResponse.error().httpStatus, 400);
  }
};

TEST_F(CollectTest, CollectSign)
{
  BankID::API::SignConfig config("127.0.0.1", BankID::Base64::encode("Test userVisibleData data"));
  config.setUserVisibleDataFormat("simpleMarkdownV1");
  EXPECT_TRUE(config.getReturnUrl().value_or("").empty());
  EXPECT_EQ(config.getUserVisibleData(), BankID::Base64::encode("Test userVisibleData data"));

  // Make the API call
  auto response = session->sign(config);
  EXPECT_TRUE(response.has_value());

  // Check the response
  const auto &orderResponse = response.value();
  EXPECT_EQ(orderResponse.httpStatus, 200);
  EXPECT_FALSE(orderResponse.orderRef.empty());
  EXPECT_FALSE(orderResponse.autoStartToken.empty());

  // Collect the order
  BankID::API::CollectConfig collectConfig(orderResponse.orderRef);
  auto collectResponse = session->collect(collectConfig);
  EXPECT_TRUE(collectResponse.has_value());
  EXPECT_EQ(collectResponse.value().httpStatus, 200);
  EXPECT_EQ(collectResponse.value().orderRef, orderResponse.orderRef);
  EXPECT_EQ(collectResponse.value().status, BankID::API::CollectStatus::PENDING);

  // Cancel the order
  BankID::API::CancelConfig cancelConfig(orderResponse.orderRef);
  auto cancelResponse = session->cancel(cancelConfig);
  EXPECT_TRUE(cancelResponse.has_value());
  EXPECT_EQ(cancelResponse.value().httpStatus, 200);

  // Try to collect again after cancellation
  collectResponse = session->collect(collectConfig);
  EXPECT_FALSE(collectResponse.has_value());

  if (!collectResponse.has_value())
  {
    EXPECT_EQ(collectResponse.error().details, R"({"errorCode":"invalidParameters","details":"No such order"})");
    EXPECT_EQ(collectResponse.error().errorCode, BankID::BankIdErrorCode::INVALID_PARAMETERS);
    EXPECT_EQ(collectResponse.error().httpStatus, 400);
  }
};

TEST_F(CollectTest, CollectPhoneSign)
{
  BankID::API::PhoneSignConfig config("RP", BankID::Base64::encode("Test userVisibleData data"));
  config.setUserVisibleDataFormat("simpleMarkdownV1");
  config.setPersonalNumber("200003121145");

  EXPECT_EQ(config.getUserVisibleData(), BankID::Base64::encode("Test userVisibleData data"));

  // Make the API call
  auto response = session->phoneSign(config);
  EXPECT_TRUE(response.has_value());

  // Check the response
  const auto &orderResponse = response.value();
  EXPECT_EQ(orderResponse.httpStatus, 200);
  EXPECT_FALSE(orderResponse.orderRef.empty());

  // Collect the order
  BankID::API::CollectConfig collectConfig(orderResponse.orderRef);
  auto collectResponse = session->collect(collectConfig);
  EXPECT_TRUE(collectResponse.has_value());
  EXPECT_EQ(collectResponse.value().httpStatus, 200);
  EXPECT_EQ(collectResponse.value().orderRef, orderResponse.orderRef);
  EXPECT_EQ(collectResponse.value().status, BankID::API::CollectStatus::PENDING);

  // Cancel the order
  BankID::API::CancelConfig cancelConfig(orderResponse.orderRef);
  auto cancelResponse = session->cancel(cancelConfig);
  EXPECT_TRUE(cancelResponse.has_value());
  EXPECT_EQ(cancelResponse.value().httpStatus, 200);

  // Try to collect again after cancellation
  collectResponse = session->collect(collectConfig);
  EXPECT_FALSE(collectResponse.has_value());

  if (!collectResponse.has_value())
  {
    EXPECT_EQ(collectResponse.error().details, R"({"errorCode":"invalidParameters","details":"No such order"})");
    EXPECT_EQ(collectResponse.error().errorCode, BankID::BankIdErrorCode::INVALID_PARAMETERS);
    EXPECT_EQ(collectResponse.error().httpStatus, 400);
  }
};
