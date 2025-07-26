#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "bankid.h"
#include "api/auth.h"
#include "api/responses.h"

#include <iostream>
#include <fstream>

class SSLConfigTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
  }

  void TearDown() override
  {
  }
};

TEST_F(SSLConfigTest, BasicDefaultValidConfig)
{
  BankID::SSLConfig sslConfig(BankID::Environment::TEST);

  EXPECT_EQ(sslConfig.environment, BankID::Environment::TEST);
  EXPECT_TRUE(sslConfig.validate());
}

TEST_F(SSLConfigTest, BasicDefaultInvalidConfig)
{
  BankID::SSLConfig sslConfig(BankID::Environment::TEST, "certs/invalid.ca",
                              "certs/bankid_cert.pem", "certs/bankid_key.pem");

  EXPECT_NE(sslConfig.environment, BankID::Environment::PRODUCTION);
  EXPECT_FALSE(sslConfig.validate());
}

TEST_F(SSLConfigTest, CustomConfig)
{
  BankID::SSLConfig sslConfig(BankID::Environment::PRODUCTION, "certs/prod.ca",
                              "certs/bankid_cert.pem", "certs/bankid_key.pem");

  EXPECT_EQ(sslConfig.environment, BankID::Environment::PRODUCTION);
  EXPECT_TRUE(sslConfig.validate());
}

TEST_F(SSLConfigTest, SendApiRequestWithValidConfig)
{
  BankID::SSLConfig sslConfig(BankID::Environment::TEST);

  EXPECT_TRUE(sslConfig.validate());

  BankID::Session session(sslConfig);
  EXPECT_TRUE(session.isInitialized());

  BankID::API::AuthConfig authConfig("127.0.0.1");
  EXPECT_TRUE(authConfig.getEndUserIp() == "127.0.0.1");

  auto response = session.auth(authConfig);
  EXPECT_TRUE(response.has_value());

  EXPECT_EQ(response.value().httpStatus, 200);
  EXPECT_TRUE(!response.value().orderRef.empty());

  const BankID::API::CancelConfig cancelConfig(response.value().orderRef);
  EXPECT_TRUE(cancelConfig.getOrderRef() == response.value().orderRef);

  auto cancelResponse = session.cancel(cancelConfig);
  EXPECT_TRUE(cancelResponse.has_value());
  EXPECT_EQ(cancelResponse.value().httpStatus, 200);
}

TEST_F(SSLConfigTest, SendApiRequestWithInvalidConfig)
{
  BankID::SSLConfig sslConfig(BankID::Environment::TEST, "certs/prod.ca",
                              "certs/bankid_cert.pem", "certs/bankid_key.pem");

  EXPECT_TRUE(sslConfig.validate());

  BankID::Session session(sslConfig);
  EXPECT_TRUE(session.isInitialized());

  BankID::API::AuthConfig authConfig("127.0.0.1");
  EXPECT_TRUE(authConfig.getEndUserIp() == "127.0.0.1");

  auto response = session.auth(authConfig);
  EXPECT_FALSE(response.has_value());

  if (!response.has_value())
  {
    EXPECT_EQ(response.error().httpStatus, 403);
    EXPECT_EQ(response.error().errorCode, BankID::BankIdErrorCode::INTERNAL_ERROR);
    EXPECT_EQ(response.error().details, "SSL server verification failed");
  }
}