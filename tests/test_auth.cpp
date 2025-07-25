#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "bankid.h"
#include "api/auth.h"
#include "api/responses.h"

class AuthTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // Setup SSL config for testing
    sslConfig = BankID::SSLConfig(BankID::Environment::TEST);

    // Create session
    session = std::make_unique<BankID::Session>(sslConfig);
  }

  void TearDown() override
  {
    session.reset();
  }

  BankID::SSLConfig sslConfig;
  std::unique_ptr<BankID::Session> session;
};

TEST_F(AuthTest, CreateAuthConfigWithPersonalNumber)
{
  BankID::API::AuthConfig config("192.168.1.1");

  BankID::Requirement requirement;
  requirement.personalNumber = "190000000000";
  config.setRequirement(requirement);

  EXPECT_EQ(config.getEndUserIp(), "192.168.1.1");
  EXPECT_TRUE(config.getRequirement().has_value());
  EXPECT_EQ(config.getRequirement().value().personalNumber.value(), "190000000000");
}

TEST_F(AuthTest, CreateAuthConfigWithoutPersonalNumber)
{
  BankID::API::AuthConfig config("192.168.1.1");

  EXPECT_EQ(config.getEndUserIp(), "192.168.1.1");
  EXPECT_FALSE(config.getRequirement().has_value());
}

TEST_F(AuthTest, CreateAuthConfigWithRequirement)
{
  BankID::API::AuthConfig config("192.168.1.1");

  BankID::Requirement requirement;
  requirement.cardReader = "class1";
  requirement.certificatePolicies = {"1.2.752.78.1.1", "1.2.752.78.1.2"};
  config.setRequirement(requirement);

  EXPECT_EQ(config.getEndUserIp(), "192.168.1.1");
  EXPECT_TRUE(config.getRequirement().has_value());
  EXPECT_EQ(config.getRequirement().value().cardReader.value(), "class1");
  EXPECT_EQ(config.getRequirement().value().certificatePolicies.value().size(), 2);
}

TEST_F(AuthTest, CreateAuthConfigWithUserVisibleData)
{
  BankID::API::AuthConfig config("192.168.1.1");
  config.setUserVisibleData("VGVzdCBkYXRh") // Base64 encoded "Test data"
      .setUserVisibleDataFormat("simpleMarkdownV1");

  EXPECT_EQ(config.getEndUserIp(), "192.168.1.1");
  EXPECT_TRUE(config.getUserVisibleData().has_value());
  EXPECT_EQ(config.getUserVisibleData().value(), "VGVzdCBkYXRh");
  EXPECT_TRUE(config.getUserVisibleDataFormat().has_value());
  EXPECT_EQ(config.getUserVisibleDataFormat().value(), "simpleMarkdownV1");
}

TEST_F(AuthTest, ValidateEndUserIpRequired)
{
  BankID::API::AuthConfig config1("192.168.1.1");
  EXPECT_EQ(config1.getEndUserIp(), "192.168.1.1");

  BankID::API::AuthConfig config2("");
  EXPECT_TRUE(config2.getEndUserIp().empty());
}

TEST_F(AuthTest, AuthWithInvalidSSLConfig)
{
  BankID::SSLConfig invalidConfig(BankID::Environment::TEST, "invalid_cert.pem", "invalid_key.pem");

  EXPECT_FALSE(invalidConfig.validate());

  BankID::Session invalidSession(invalidConfig);
  EXPECT_FALSE(invalidSession.isInitialized());

  BankID::API::AuthConfig config("192.168.1.1");

  auto result = invalidSession.auth(config);
  EXPECT_EQ(result.has_value(), 0);
}

TEST_F(AuthTest, AuthWithValidConfig)
{
  BankID::API::AuthConfig config("192.168.1.1");

  BankID::Requirement requirement;
  requirement.personalNumber = "190000000000"; // Test personal number
  config.setRequirement(requirement);

  auto result = session->auth(config);

  if (result.has_value())
  {
    EXPECT_FALSE(result.value().orderRef.empty());
    EXPECT_FALSE(result.value().autoStartToken.empty());
  }
}

TEST_F(AuthTest, AuthQRCodeFlow)
{
  BankID::API::AuthConfig config("192.168.1.1");

  auto result = session->auth(config);

  if (result.has_value())
  {
    EXPECT_FALSE(result.value().orderRef.empty());
    EXPECT_FALSE(result.value().qrStartToken.empty());
  }
}

TEST_F(AuthTest, AuthWithUserVisibleData)
{
  BankID::API::AuthConfig config("192.168.1.1");
  config.setUserVisibleData("VGVzdCBkYXRh") // Base64 encoded "Test data"
      .setUserVisibleDataFormat("simpleMarkdownV1");

  auto result = session->auth(config);

  if (result.has_value())
  {
    EXPECT_FALSE(result.value().orderRef.empty());
  }
}
