#include <iostream>
#include <chrono>
#include <ctime>

#include "jwt/jwt.hpp"
#include "gtest/gtest.h"

TEST (DecodeVerifyExp, BeforeExpiryTest)
{
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret")};
  obj.add_claim("iss", "arun.muralidharan")
     .add_claim("exp", std::chrono::system_clock::now() + std::chrono::seconds{10})
     ;

  auto enc_str = obj.signature();

  auto dec_obj = jwt::decode(enc_str, algorithms({"HS256"}), secret("secret"), verify(true));
}

TEST (DecodeVerifyExp, AfterExpiryTest)
{
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret")};
  obj.add_claim("iss", "arun.muralidharan")
     .add_claim("exp", std::chrono::system_clock::now() - std::chrono::seconds{1})
     ;

  auto enc_str = obj.signature();

  EXPECT_THROW (jwt::decode(enc_str, algorithms({"HS256"}), secret("secret"), verify(true)),
                jwt::TokenExpiredError);

}

TEST (DecodeVerifyExp, AfterExpiryWithLeeway)
{
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret")};
  obj.add_claim("iss", "arun.muralidharan")
     .add_claim("exp", std::chrono::system_clock::now() - std::chrono::seconds{1})
     ;

  auto enc_str = obj.signature();
  auto dec_obj = jwt::decode(enc_str, algorithms({"HS256"}), secret("secret"), verify(true), leeway(2));
  (void)dec_obj;
}

TEST (DecodeVerifyExp, ValidIssuerTest)
{
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret")};
  obj.add_claim("iss", "arun.muralidharan")
     .add_claim("sub", "test")
     ;

  auto enc_str = obj.signature();

  auto dec_obj = jwt::decode(enc_str, algorithms({"HS256"}), secret("secret"), issuer("arun.muralidharan"));
  (void)dec_obj;
}

TEST (DecodeVerifyExp, InvalidIssuerTest_1)
{
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret"), payload({{"sub", "test"}})};
  auto enc_str = obj.signature();

  EXPECT_THROW (jwt::decode(enc_str, algorithms({"HS256"}), secret("secret"), issuer("arun.muralidharan")),
                jwt::InvalidIssuerError);

}

TEST (DecodeVerifyExp, InvalidIssuerTest_2)
{
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret"), payload({{"sub", "test"}})};
  obj.add_claim("iss", "arun.muralidharan");

  auto enc_str = obj.signature();

  EXPECT_THROW (jwt::decode(enc_str, algorithms({"HS256"}), secret("secret"), issuer("arun.murali")),
                jwt::InvalidIssuerError);
}

TEST (DecodeVerifyExp, NotImmatureSignatureTest)
{
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret"), payload({{"sub", "test"}})};
  obj.add_claim(jwt::registered_claims::not_before, std::chrono::system_clock::now() - std::chrono::seconds{10});

  auto enc_str = obj.signature();

  auto dec_obj = jwt::decode(enc_str, algorithms({"HS256"}), secret("secret"));
  (void)dec_obj;
}

TEST (DecodeVerifyExp, ImmatureSignatureTest)
{
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret"), payload({{"sub", "test"}})};
  obj.add_claim(jwt::registered_claims::not_before, std::chrono::system_clock::now() + std::chrono::seconds{10});

  auto enc_str = obj.signature();

  EXPECT_THROW (jwt::decode(enc_str, algorithms({"HS256"}), secret("secret")),
                jwt::ImmatureSignatureError);
}

TEST (DecodeVerifyExp, ImmatureSignatureTestWithLeeway)
{
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret"), payload({{"sub", "test"}})};
  obj.add_claim(jwt::registered_claims::not_before, std::chrono::system_clock::now() + std::chrono::seconds{10});

  auto enc_str = obj.signature();

  auto dec_obj = jwt::decode(enc_str, algorithms({"HS256"}), secret("secret"), leeway(10));
  (void)dec_obj;
}

TEST (DecodeVerifyExp, InvalidAudienceTest)
{
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret"), payload({{"sub", "test"}, {"aud", "www"}})};

  auto enc_str = obj.signature();

  EXPECT_THROW (jwt::decode(enc_str, algorithms({"HS256"}), secret("secret"), aud("ww")),
                jwt::InvalidAudienceError);
}

TEST (DecodeVerifyExp, InvalidSignatureTest)
{
  using namespace jwt::params;

  const char* inv_enc_str =
    "eyJhbGciOiJOT05FIiwidHlwIjoiSldUIn0.eyJhdWQiOiJyaWZ0LmlvIiwiZXhwIjoxNTEzODYzMzcxLCJzdWIiOiJub3RoaW5nIG11Y2gifQ";

  EXPECT_THROW (jwt::decode(inv_enc_str, algorithms({"none", "HS256"})),
                jwt::SignatureFormatError);
}

TEST (DecodeVerifyExp, KeyNotPresentTest)
{
  using namespace jwt::params;

  const char* enc_str =
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
    "eyJpYXQiOjE1MTM4NjIzNzEsImlkIjoiYS1iLWMtZC1lLWYtMS0yLTMiLCJpc3MiOiJhcnVuLm11cmFsaWRoYXJhbiIsInN1YiI6ImFkbWluIn0."
    "jk7bRQKTLvs1RcuvMc2B_rt6WBYPoVPirYi_QRBPiuk";

  EXPECT_THROW (jwt::decode(enc_str, algorithms({"none", "HS256"}), verify(true)),
                jwt::KeyNotPresentError);
}

TEST (DecodeVerifyExp, InvalidSubjectTest)
{
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret"), payload({{"sub", "test"}, {"aud", "www"}})};

  auto enc_str = obj.signature();

  EXPECT_THROW (jwt::decode(enc_str, algorithms({"HS256"}), secret("secret"), sub("TEST")),
                jwt::InvalidSubjectError);
}

