#include <iostream>
#include "gtest/gtest.h"
#include "jwt/jwt.hpp"

TEST (DecodeTest, InvalidFinalDotForNoneAlg)
{
  using namespace jwt::params;
  const char* inv_enc_str =
    "eyJhbGciOiJOT05FIiwidHlwIjoiSldUIn0.eyJhdWQiOiJyaWZ0LmlvIiwiZXhwIjoxNTEzODYzMzcxLCJzdWIiOiJub3RoaW5nIG11Y2gifQ";

  std::error_code ec;
  auto obj = jwt::decode(inv_enc_str, algorithms({"none", "HS256"}), ec);

  ASSERT_TRUE (ec);
  EXPECT_EQ (ec.value(), static_cast<int>(jwt::DecodeErrc::SignatureFormatError));
}

TEST (DecodeTest, DecodeNoneAlgSign)
{
  using namespace jwt::params;
  const char* enc_str =
    "eyJhbGciOiJOT05FIiwidHlwIjoiSldUIn0.eyJhdWQiOiJyaWZ0LmlvIiwiZXhwIjo0NTEzODYzMzcxLCJzdWIiOiJub3RoaW5nIG11Y2gifQ.";

  std::error_code ec;
  auto obj = jwt::decode(enc_str, algorithms({"none"}), ec, verify(true));
  EXPECT_TRUE (ec);
  EXPECT_EQ (ec.value(), static_cast<int>(jwt::AlgorithmErrc::NoneAlgorithmUsed));

  std::cout << obj.payload() << std::endl;

  EXPECT_FALSE (obj.has_claim("iss"));
  EXPECT_FALSE (obj.has_claim("ISS"));

  EXPECT_TRUE (obj.has_claim("aud"));
  EXPECT_TRUE (obj.has_claim("exp"));

  EXPECT_EQ (obj.payload().get_claim_value<uint64_t>("exp"), static_cast<uint64_t>(4513863371));
}

TEST (DecodeTest, DecodeWrongAlgo)
{
  using namespace jwt::params;

  const char* enc_str =
    "eyJhbGciOiJOT05FIiwidHlwIjoiSldUIn0.eyJhdWQiOiJyaWZ0LmlvIiwiZXhwIjoxNTEzODYzMzcxLCJzdWIiOiJub3RoaW5nIG11Y2gifQ.";

  std::error_code ec;
  auto obj = jwt::decode(enc_str, algorithms({"HS256"}), ec, secret(""), verify(true));
  EXPECT_TRUE (ec);
  EXPECT_EQ (ec.value(), static_cast<int>(jwt::VerificationErrc::InvalidAlgorithm));
}

TEST (DecodeTest, DecodeInvalidHeader)
{
  using namespace jwt::params;

  const char* enc_str =
    "ehbGciOiJOT05FIiwidHlwIjoiSldUIn0.eyJhdWQiOiJyaWZ0LmlvIiwiZXhwIjoxNTEzODYzMzcxLCJzdWIiOiJub3RoaW5nIG11Y2gifQ.";

  std::error_code ec;
  auto obj = jwt::decode(enc_str, algorithms({"HS256"}), ec, secret(""), verify(true));
  ASSERT_TRUE (ec);
  EXPECT_EQ (ec.value(), static_cast<int>(jwt::DecodeErrc::JsonParseError));

}

TEST (DecodeTest, DecodeEmptyHeader)
{
  using namespace jwt::params;

  const char* enc_str =
    ".eyJhdWQiOiJyaWZ0LmlvIiwiZXhwIjoxNTEzODYzMzcxLCJzdWIiOiJub3RoaW5nIG11Y2gifQ.";

  std::error_code ec;
  auto obj = jwt::decode(enc_str, algorithms({"HS256"}), ec, secret(""), verify(true));
  ASSERT_TRUE (ec);
  EXPECT_EQ (ec.value(), static_cast<int>(jwt::DecodeErrc::JsonParseError));

}

TEST (DecodeTest, DecodeInvalidPayload)
{
  using namespace jwt::params;

  const char* enc_str =
    "eyJhbGciOiJOT05FIiwidHlwIjoiSldUIn0.eyfhuWcikiJyaWZ0LmlvIiwiZXhwIsexNTEzODYzMzcxLCJzdWIiOiJub3RoaW5nIG11Y2gifQ.";

  std::error_code ec;
  auto obj = jwt::decode(enc_str, algorithms({"none"}), ec, verify(true));
  ASSERT_TRUE (ec);

  EXPECT_EQ (ec.value(), static_cast<int>(jwt::DecodeErrc::JsonParseError));
}

TEST (DecodeTest, DecodeHS256)
{
  using namespace jwt::params;

  const char* enc_str =
   "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
   "eyJpYXQiOjE1MTM4NjIzNzEsImlkIjoiYS1iLWMtZC1lLWYtMS0yLTMiLCJpc3MiOiJhcnVuLm11cmFsaWRoYXJhbiIsInN1YiI6ImFkbWluIn0."
   "jk7bRQKTLvs1RcuvMc2B_rt6WBYPoVPirYi_QRBPiuk";

  std::error_code ec;
  auto obj = jwt::decode(enc_str, algorithms({"none", "HS256"}), ec, verify(false), secret("secret"));
  ASSERT_FALSE (ec);

  EXPECT_TRUE (obj.has_claim("iss"));
  EXPECT_TRUE (obj.payload().has_claim_with_value("iss", "arun.muralidharan"));

  //Case sensitive search
  EXPECT_FALSE (obj.has_claim("IAT"));
  EXPECT_TRUE (obj.payload().has_claim_with_value(jwt::registered_claims::issued_at, 1513862371));

  EXPECT_FALSE (obj.payload().has_claim_with_value(jwt::registered_claims::issued_at, 1513862372));
}

TEST (DecodeTest, SecretKeyNotPassed)
{
  using namespace jwt::params;

  const char* enc_str =
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
    "eyJpYXQiOjE1MTM4NjIzNzEsImlkIjoiYS1iLWMtZC1lLWYtMS0yLTMiLCJpc3MiOiJhcnVuLm11cmFsaWRoYXJhbiIsInN1YiI6ImFkbWluIn0."
    "jk7bRQKTLvs1RcuvMc2B_rt6WBYPoVPirYi_QRBPiuk";

  std::error_code ec;
  auto obj = jwt::decode(enc_str, algorithms({"none", "HS256"}), ec, verify(true));

  ASSERT_TRUE (ec);
  EXPECT_EQ (ec.value(), static_cast<int>(jwt::DecodeErrc::KeyNotPresent));
}

TEST (DecodeTest, DecodeHS384)
{
  using namespace jwt::params;

  const char* enc_str =
    "eyJhbGciOiJIUzM4NCIsInR5cCI6IkpXVCJ9."
    "eyJhdWQiOiJyaWZ0LmlvIiwiZXhwIjoxNTEzODYzMzcxLCJzdWIiOiJub3RoaW5nIG11Y2gifQ."
    "cGN4FZCe9Y2c1dA-jP71IXGnYbJRc4OaUTa5m7N7ybF5h6wBwxWQ-pdcxYchjDBL";

  const jwt::string_view key = "0123456789abcdefghijklmnopqrstuvwxyz";

  std::error_code ec;
  auto obj = jwt::decode(enc_str, algorithms({"none", "HS384"}), ec, verify(false), secret(key));
  ASSERT_FALSE (ec);

  EXPECT_TRUE (obj.has_claim("sub"));
  EXPECT_TRUE (obj.payload().has_claim_with_value("sub", "nothing much"));
}

TEST (DecodeTest, DecodeHS512)
{
  using namespace jwt::params;

  const char* enc_str =
  "eyJhbGciOiJIUzUxMiIsInR5cCI6IkpXVCJ9."
  "eyJhdWQiOiJyaWZ0LmlvIiwiZXhwIjoxNTEzODYzMzcxLCJzdWIiOiJub3RoaW5nIG11Y2gifQ."
  "vQ-1JSFN1kPjUI3URP6AFK5z8V7xLhyhw-76QWhQg9Xcy-IgrJ-bCTYLBjgaprrcEWwpSnBQnP3QnIxYK0HEaQ";

  const jwt::string_view key = "00112233445566778899";

  std::error_code ec;
  auto obj = jwt::decode(enc_str, algorithms({"none", "HS384", "HS512"}), ec, verify(false), secret(key));

  ASSERT_FALSE (ec);

  EXPECT_TRUE (obj.has_claim("sub"));
  EXPECT_TRUE (obj.payload().has_claim_with_value("sub", "nothing much"));
}

TEST (DecodeTest, TypHeaderMiss)
{
  using namespace jwt::params;

  const char* enc_str =
    "eyJhbGciOiJIUzI1NiJ9."
    "eyJleHAiOjE1MzM0NjE1NTMsImlhdCI6MTUxMzg2MjM3MSwiaWQiOiJhLWItYy1kLWUtZi0xLTItMyIsImlzcyI6ImFydW4ubXVyYWxpZGhhcmFuIiwic3ViIjoiYWRtaW4ifQ."
    "pMWBLSWl1p4V958lfe_6ZhvgFMOQv9Eq5mlndVKFKkA";

  std::error_code ec;
  auto obj = jwt::decode(enc_str, algorithms({"none", "HS256"}), ec, verify(false));
  std::cout << "Decode header: " << obj.header() << std::endl;

  EXPECT_FALSE (ec);
}

