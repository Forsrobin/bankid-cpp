#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "bankid.h"
#include "api/auth.h"
#include "api/responses.h"

class CancelTest : public ::testing::Test
{
public:
  CancelTest()
      : sslConfig(BankID::Environment::TEST),
        session(std::make_unique<BankID::Session>(sslConfig))
  {
  }

protected:
  void TearDown() override
  {
    session.reset();
  }

  BankID::SSLConfig sslConfig;
  std::unique_ptr<BankID::Session> session;
};

TEST_F(CancelTest, CancelAuth)
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

  BankID::API::CancelConfig cancelConfig(orderResponse.orderRef);
  auto cancelResponse = session->cancel(cancelConfig);
  EXPECT_TRUE(cancelResponse.has_value());
  EXPECT_EQ(cancelResponse.value().httpStatus, 200);
}

TEST_F(CancelTest, CancelSign)
{
  BankID::API::SignConfig config("127.0.0.1", BankID::Base64::encode("Test Sign Data"));

  EXPECT_TRUE(config.getReturnUrl().value_or("").empty());
  EXPECT_FALSE(config.getUserVisibleData().empty());

  // Make the API call
  auto response = session->sign(config);

  EXPECT_TRUE(response.has_value());

  // Check the response
  const auto &orderResponse = response.value();
  EXPECT_EQ(orderResponse.httpStatus, 200);
  EXPECT_FALSE(orderResponse.orderRef.empty());

  BankID::API::CancelConfig cancelConfig(orderResponse.orderRef);
  auto cancelResponse = session->cancel(cancelConfig);
  EXPECT_TRUE(cancelResponse.has_value());
  EXPECT_EQ(cancelResponse.value().httpStatus, 200);
}

TEST_F(CancelTest, CancelPayment)
{

  BankID::API::UserVisibleTransaction transaction;
  transaction.transactionType = "card";
  transaction.recipient.name = "Test Recipient";
  transaction.money = BankID::API::PaymentMoney{"100,00", BankID::API::CurrencyCode::EUR};

  BankID::API::PaymentConfig config("127.0.0.1", transaction);

  EXPECT_TRUE(config.getReturnUrl().value_or("").empty());
  EXPECT_TRUE(config.getUserVisibleData().value_or("").empty());

  // Make the API call
  auto response = session->payment(config);

  EXPECT_TRUE(response.has_value());

  // Check the response
  const auto &orderResponse = response.value();
  EXPECT_EQ(orderResponse.httpStatus, 200);
  EXPECT_FALSE(orderResponse.orderRef.empty());

  BankID::API::CancelConfig cancelConfig(orderResponse.orderRef);
  auto cancelResponse = session->cancel(cancelConfig);
  EXPECT_TRUE(cancelResponse.has_value());
  EXPECT_EQ(cancelResponse.value().httpStatus, 200);
}
