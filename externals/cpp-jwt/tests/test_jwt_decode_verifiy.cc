#include <iostream>
#include <chrono>
#include <ctime>

#include "jwt/jwt.hpp"
#include "gtest/gtest.h"

TEST (DecodeVerify, BeforeExpiryTest)
{
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret")};
  obj.add_claim("iss", "arun.muralidharan")
     .add_claim("exp", std::chrono::system_clock::now() + std::chrono::seconds{10})
     ;

  std::error_code ec;
  auto enc_str = obj.signature(ec);

  ASSERT_FALSE (ec);

  auto dec_obj = jwt::decode(enc_str, algorithms({"HS256"}), ec, secret("secret"), verify(true));
  ASSERT_FALSE (ec);
}

TEST (DecodeVerify, AfterExpiryTest)
{
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret")};
  obj.add_claim("iss", "arun.muralidharan")
     .add_claim("exp", std::chrono::system_clock::now() - std::chrono::seconds{1})
     ;

  std::error_code ec;
  auto enc_str = obj.signature(ec);
  ASSERT_FALSE (ec);

  auto dec_obj = jwt::decode(enc_str, algorithms({"HS256"}), ec, secret("secret"), verify(true));
  ASSERT_TRUE (ec);
  EXPECT_EQ (ec.value(), static_cast<int>(jwt::VerificationErrc::TokenExpired));
}

TEST (DecodeVerify, AfterExpiryWithLeeway)
{
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret")};
  obj.add_claim("iss", "arun.muralidharan")
     .add_claim("exp", std::chrono::system_clock::now() - std::chrono::seconds{1})
     ;

  std::error_code ec;
  auto enc_str = obj.signature(ec);
  ASSERT_FALSE (ec);

  auto dec_obj = jwt::decode(enc_str, algorithms({"HS256"}), ec, secret("secret"), verify(true), leeway(2));
  ASSERT_FALSE (ec);
}

TEST (DecodeVerify, ValidIssuerTest)
{
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret")};
  obj.add_claim("iss", "arun.muralidharan")
     .add_claim("sub", "test")
     ;

  std::error_code ec;
  auto enc_str = obj.signature(ec);
  ASSERT_FALSE (ec);

  auto dec_obj = jwt::decode(enc_str, algorithms({"HS256"}), ec, secret("secret"), issuer("arun.muralidharan"));
  ASSERT_FALSE (ec);
}

TEST (DecodeVerify, InvalidIssuerTest_1)
{
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret"), payload({{"sub", "test"}})};
  std::error_code ec;
  auto enc_str = obj.signature(ec);
  ASSERT_FALSE (ec);

  auto dec_obj = jwt::decode(enc_str, algorithms({"HS256"}), ec, secret("secret"), issuer("arun.muralidharan"));
  ASSERT_TRUE (ec);

  EXPECT_EQ (ec.value(), static_cast<int>(jwt::VerificationErrc::InvalidIssuer));
}

TEST (DecodeVerify, InvalidIssuerTest_2)
{
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret"), payload({{"sub", "test"}})};
  obj.add_claim("iss", "arun.muralidharan");

  std::error_code ec;
  auto enc_str = obj.signature(ec);
  ASSERT_FALSE (ec);

  auto dec_obj = jwt::decode(enc_str, algorithms({"HS256"}), ec, secret("secret"), issuer("arun.murali"));
  ASSERT_TRUE (ec);
  EXPECT_EQ (ec.value(), static_cast<int>(jwt::VerificationErrc::InvalidIssuer));
}

TEST (DecodeVerify, NotImmatureSignatureTest)
{
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret"), payload({{"sub", "test"}})};
  obj.add_claim(jwt::registered_claims::not_before, std::chrono::system_clock::now() - std::chrono::seconds{10});

  std::error_code ec;
  auto enc_str = obj.signature(ec);
  ASSERT_FALSE (ec);

  auto dec_obj = jwt::decode(enc_str, algorithms({"HS256"}), ec, secret("secret"));
  ASSERT_FALSE (ec);
}

TEST (DecodeVerify, ImmatureSignatureTest)
{
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret"), payload({{"sub", "test"}})};
  obj.add_claim(jwt::registered_claims::not_before, std::chrono::system_clock::now() + std::chrono::seconds{10});

  std::error_code ec;
  auto enc_str = obj.signature(ec);
  ASSERT_FALSE (ec);

  auto dec_obj = jwt::decode(enc_str, algorithms({"HS256"}), ec, secret("secret"));
  ASSERT_TRUE (ec);
  EXPECT_EQ (ec.value(), static_cast<int>(jwt::VerificationErrc::ImmatureSignature));
}

TEST (DecodeVerify, ImmatureSignatureTestWithLeeway)
{
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret"), payload({{"sub", "test"}})};
  obj.add_claim(jwt::registered_claims::not_before, std::chrono::system_clock::now() + std::chrono::seconds{10});

  std::error_code ec;
  auto enc_str = obj.signature(ec);
  ASSERT_FALSE (ec);

  auto dec_obj = jwt::decode(enc_str, algorithms({"HS256"}), ec, secret("secret"), leeway(10));
  ASSERT_FALSE (ec);
}

TEST (DecodeVerify, InvalidAudienceTest)
{
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret"), payload({{"sub", "test"}, {"aud", "www"}})};

  std::error_code ec;
  auto enc_str = obj.signature(ec);
  ASSERT_FALSE (ec);

  auto dec_obj = jwt::decode(enc_str, algorithms({"HS256"}), ec, secret("secret"), aud("ww"));
  ASSERT_TRUE (ec);
  EXPECT_EQ (ec.value(), static_cast<int>(jwt::VerificationErrc::InvalidAudience));
}

TEST (DecodeVerify, InvalidIATTest)
{
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret"), payload({{"sub", "test"}, {"aud", "www"}})};

  obj.add_claim("iat", "what?");
  auto enc_str = obj.signature();

  std::error_code ec;
  auto dec_obj = jwt::decode(enc_str, algorithms({"HS256"}), ec, secret("secret"), validate_iat(true));
  EXPECT_EQ (ec.value(), static_cast<int>(jwt::VerificationErrc::TypeConversionError));
}

TEST (DecodeVerify, InvalidSignatureTest)
{
  using namespace jwt::params;

  std::error_code ec;
  auto dec_obj = jwt::decode("", algorithms({"HS256"}), ec, secret("secret"), validate_iat(true));
  EXPECT_EQ (ec.value(), static_cast<int>(jwt::DecodeErrc::SignatureFormatError));

  ec.clear();
  dec_obj = jwt::decode("abcdsdfhbsdhjfbsdj.", algorithms({"HS256"}), ec, secret("secret"), validate_iat(true));
  EXPECT_EQ (ec.value(), static_cast<int>(jwt::DecodeErrc::SignatureFormatError));
}

