#include <iostream>
#include <fstream>
#include <map>
#include <chrono>

#include "gtest/gtest.h"
#include "jwt/jwt.hpp"

#define EC384_PUB_KEY CERT_ROOT_DIR "/ec_certs/ec384_pub.pem"
#define EC384_PRIV_KEY CERT_ROOT_DIR "/ec_certs/ec384_priv.pem"

std::string read_from_file(const std::string& path)
{
  std::string contents;
  std::ifstream is{path, std::ifstream::binary};

  if (is) {
    // get length of file:
    is.seekg (0, is.end);
    auto length = is.tellg();
    is.seekg (0, is.beg);
    contents.resize(length);

    is.read(&contents[0], length);
    if (!is) {
      is.close();
      return {};
    }
  }

  is.close();
  return contents;
}

TEST (ESAlgo, ES256EncodingDecodingTest)
{
  using namespace jwt::params;

  std::string key = read_from_file(EC384_PRIV_KEY);
  ASSERT_TRUE (key.length());

  jwt::jwt_object obj{algorithm("ES256"), secret(key)};

  obj.add_claim("iss", "arun.muralidharan")
     .add_claim("aud", "all")
     .add_claim("exp", 1513862371)
     ;

  std::error_code ec;
  auto enc_str = obj.signature(ec);
  EXPECT_FALSE (ec);

  key = read_from_file(EC384_PUB_KEY);
  ASSERT_TRUE (key.length());

  auto dec_obj = jwt::decode(enc_str, algorithms({"ES256"}), ec, verify(false), secret(key));
  EXPECT_FALSE (ec);

  EXPECT_EQ (dec_obj.header().algo(), jwt::algorithm::ES256);
  EXPECT_TRUE (dec_obj.has_claim("iss"));
  EXPECT_TRUE (dec_obj.has_claim("aud"));
  EXPECT_TRUE (dec_obj.has_claim("exp"));

  EXPECT_FALSE (dec_obj.has_claim("sub"));
}

TEST (ESAlgo, ES384EncodingDecodingTest)
{
  using namespace jwt::params;

  std::string key = read_from_file(EC384_PRIV_KEY);
  ASSERT_TRUE (key.length());

  jwt::jwt_object obj{algorithm("ES384"), secret(key)};

  obj.add_claim("iss", "arun.muralidharan")
     .add_claim("aud", "all")
     .add_claim("exp", 1513862371)
     ;

  auto enc_str = obj.signature();

  key = read_from_file(EC384_PUB_KEY);
  ASSERT_TRUE (key.length());

  auto dec_obj = jwt::decode(enc_str, algorithms({"ES384"}), verify(false), secret(key));

  EXPECT_EQ (dec_obj.header().algo(), jwt::algorithm::ES384);
}

TEST (ESAlgo, ES512EncodingDecodingTest)
{
  using namespace jwt::params;

  std::string key = read_from_file(EC384_PRIV_KEY);
  ASSERT_TRUE (key.length());

  jwt::jwt_object obj{algorithm("ES512"), secret(key)};

  obj.add_claim("iss", "arun.muralidharan")
     .add_claim("aud", "all")
     .add_claim("exp", 1513862371)
     ;

  auto enc_str = obj.signature();

  key = read_from_file(EC384_PUB_KEY);
  ASSERT_TRUE (key.length());

  auto dec_obj = jwt::decode(enc_str, algorithms({"ES512"}), verify(false), secret(key));

  EXPECT_EQ (dec_obj.header().algo(), jwt::algorithm::ES512);
}

TEST (ESAlgo, ES384EncodingDecodingValidTest)
{
  using namespace jwt::params;

  std::string key = read_from_file(EC384_PRIV_KEY);
  ASSERT_TRUE (key.length());

  jwt::jwt_object obj{algorithm("ES384"), secret(key)};

  obj.add_claim("iss", "arun.muralidharan")
     .add_claim("aud", "all")
     .add_claim("exp", 4682665886) // Expires on Sunday, May 22, 2118 12:31:26 PM GMT
     ;

  auto enc_str = obj.signature();

  key = read_from_file(EC384_PUB_KEY);
  ASSERT_TRUE (key.length());

  auto dec_obj = jwt::decode(enc_str, algorithms({"ES384"}), verify(true), secret(key));

  EXPECT_EQ (dec_obj.header().algo(), jwt::algorithm::ES384);
  EXPECT_TRUE (dec_obj.has_claim("exp"));
  EXPECT_TRUE (obj.payload().has_claim_with_value("exp", 4682665886));

  std::map<std::string, std::string> keystore{{"arun.muralidharan", key}};

  auto l = [&keystore](const jwt::jwt_payload& payload){
    auto iss = payload.get_claim_value<std::string>("iss");
    return keystore[iss];
  };
  auto dec_obj2 = jwt::decode(enc_str, algorithms({"ES384"}), verify(true), secret(l));
  EXPECT_EQ (dec_obj2.header().algo(), jwt::algorithm::ES384);
}

