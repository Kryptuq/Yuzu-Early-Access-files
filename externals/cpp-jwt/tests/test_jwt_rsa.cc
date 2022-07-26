#include <iostream>
#include <fstream>
#include <map>
#include <chrono>

#include "gtest/gtest.h"
#include "jwt/jwt.hpp"

#define RSA256_PUB_KEY CERT_ROOT_DIR "/rsa_certs/rsa256_pub.pem"
#define RSA256_PRIV_KEY CERT_ROOT_DIR "/rsa_certs/rsa256_priv.pem"
#define RSA384_PUB_KEY CERT_ROOT_DIR "/rsa_certs/rsa384_pub.pem"
#define RSA384_PRIV_KEY CERT_ROOT_DIR "/rsa_certs/rsa384_priv.pem"
#define RSA512_PUB_KEY CERT_ROOT_DIR "/rsa_certs/rsa512_pub.pem"
#define RSA512_PRIV_KEY CERT_ROOT_DIR "/rsa_certs/rsa512_priv.pem"

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

TEST (RSAAlgo, RSA256EncodingDecodingTest)
{
  using namespace jwt::params;

  const char* expected_sign =
    "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOiJhbGwiLCJleHAiOjE1MTM4NjIzNzEsImlzcyI6ImFydW4ubXVyYWxpZGhhcmFuIn0.jr-Nrny0yGFuIUH8zHLuxpGH5aClwQVin2As2ISsgclu-9IDi1cVCtloIUNRb_ock6X7X41FtGMA_lt_T9wGyLmMzNf4Vu7OPBGfzjEdCHKD8OgcvI0Z4qw7_TFuXEuNSnbwkYFZ9S2g8uPzO0raVk4aIuczo58btwEDrsoE7TNBMTHjfL92zZ90YcFqW5WZKn9Y_dF1rb5UXARF6YSzzVjaNC86FWUl86wwo9cir0nxVPD4zKol_x2xyiP6n4n-sUX0_dM_-KMSfDqdr34quq3ZxcP5vjT-8FWb4t_IWHBmLrNsjS1so9a_5u7vcSBX1llX9Vgztv0zB7B8rEkFTw";

  std::string key = read_from_file(RSA256_PRIV_KEY);
  ASSERT_TRUE (key.length());

  jwt::jwt_object obj{algorithm("RS256"), secret(key)};

  obj.add_claim("iss", "arun.muralidharan")
     .add_claim("aud", "all")
     .add_claim("exp", 1513862371)
     ;

  std::error_code ec;
  auto enc_str = obj.signature(ec);
  EXPECT_FALSE (ec);

  EXPECT_EQ (enc_str, expected_sign);

  //Decode
  key = read_from_file(RSA256_PUB_KEY);
  ASSERT_TRUE (key.length());

  auto dec_obj = jwt::decode(enc_str, algorithms({"RS256"}), ec, verify(false), secret(key));
  EXPECT_FALSE (ec);
}

TEST (RSAAlgo, RSA384EncodingDecodingTest)
{
  using namespace jwt::params;

  std::string key = read_from_file(RSA384_PRIV_KEY);
  ASSERT_TRUE (key.length());

  const char* expected_str =
    "eyJhbGciOiJSUzM4NCIsInR5cCI6IkpXVCJ9.eyJhdWQiOiJhbGwiLCJleHAiOjE1MTM4NjIzNzIsImlzcyI6ImFydW4ubXVyYWxpZGhhcmFuIn0.iXQyGTmHAjdfXXgcMZn31xqv05h8Qoa3GGlSF5-42kPkd6iLPWzxky15FFW8qkvO-DiXDpOM4BoDANYCKNTSOToyuhCZ6dn_WH8RQzU6KOqRccYe2Fgvo7XnrgE_iHIMYPejc2kAUh1xLpE31WCU2P1afo2KN_-DV7kCmDJY6qpFtCctbbPNOhv6XbYpQlTblZeYDh1HVO--KWuhYl17kgjj3W-3fEoQjgaiprZ_JsTxRTN05aGT_AY15-FW0jPgPPBw5FnIX6P-j18F3BrG-lji7BuNrvyCUT3ZX35yBkBv9Ri5B3SLALy2bD0qGGE_G9_Orfm9yU9oQySLMO1qLiMbKLakLB5kMSy049C2Pdx9Nz47hqQWOHOWNRGwwTkKAwjeu1dTjv14QOmLcefM6GoXoCMZaFcmEqr63CgyLrnlsVS6vLkazyWcKD6eg51vPa8Rnn1V5u1EgNNnT6nU6iZ9_POJcf9_s-7HNpAXtlckia-OIrdLG-5cm93h1rAfVois43m0EwNtTr_DZ2JDtM9BifaS5MsktztUjrh1hjF5vDLBQc8vAYX0YbWOx_0NTn0aRYzOZ9kIhFxkaY320h8AS_7iFa5sA-ygeJdR-EvdlUZcoRzPzQFkrtatK-UE_VlSisUCsqoxHefx799aNjqz4FDLcyQRekdmVMb8Ew8";

  jwt::jwt_object obj{algorithm("RS384"), secret(key)};

  obj.add_claim("iss", "arun.muralidharan")
     .add_claim("aud", "all")
     .add_claim("exp", 1513862372)
     ;

  auto enc_str = obj.signature();
  EXPECT_EQ (enc_str, expected_str);

  //Decode
  key = read_from_file(RSA384_PUB_KEY);
  ASSERT_TRUE (key.length());

  auto dec_obj = jwt::decode(enc_str, algorithms({"none", "HS384", "RS384"}), verify(false), secret(key));
  EXPECT_EQ (dec_obj.header().algo(), jwt::algorithm::RS384);
}

TEST (RSAAlgo, RSA512EncodingDecodingTest)
{
  using namespace jwt::params;

  const char* expected_str =
    "eyJhbGciOiJSUzUxMiIsInR5cCI6IkpXVCJ9.eyJhdWQiOiJhbGwiLCJleHAiOjE1MTM4NjIzNzIsImlzcyI6ImFydW4ubXVyYWxpZGhhcmFuIn0.ZNkLnf565-NFfxkbosJra1CJEgCLFf0jmgb7Q8eZrzxIrE4C4dOjpGD13f0jm2FqidUxAvFVrHI2ahhZi4Bu65qQtV4mVVftiD0qTaYzh26ql0MFqTKYEeKtU0kFXAzH7f9689z7mQ2n8aw7H8WHrfe17ub19Xyj-MirCECcWjcuGWBhsdz0y-dKy_GJYnpf8mHvmQAjkH5ynUV5NXHIBDO6eKssxX36Ow9_KYZ1HrCCUT_B-TQfNrnHAJgCydO-cX9iaAxV5aKvOdMGopHz14fX4oI9qH4aBzcroRbs77UsJZ-CMoRnUoXQP7DPORvEEUOQepANu9gqmymfJin8oEDotlj7eoJkFD3j64dkMT2bnRe8k2akPgUiDTeIrvNBuOIMDJtekoVpTo0fytveeDVPpDli9uX6DkJW1GGFLSRR-J-I8WbKRMKadmKOpDn3LF71hOo2mcXAsSwleFi9xB39bLBKJcqL_DtBoZBt0QSqMs6nRVj1U-3vYtuaa_eM3TfxhWWPZULaGVaVfpefRGdqtjrU0z5oO_vjviYujXK5_vM8zTroLVEaOyJYCeh2h_5N5LaOlf8BDu2PF3epNuCmM7G2PWEH7aPn5o-vvKTg_SM32jJXbXp2gkplEdQIWFh3jtjcRe9wNa9aPJE3I1hn1ZbqiAGUzBLWYUYpvstWXGbmxOoh6FkNJERb2hOIZgGLMvwWZXUU3GICcI5DMFOdDsuANpLg5FygsQ68JpuqKrUxu1Yh55--GHuDI7tqdHsPhPUzTmZrSvRog0w07dUAZCIBsGsSLX3wViobWbpVuY4pB7KXGIfdXgLfLgcERe_CxtnoPGF36zsqBflSXcqXwJ4qRK6BpTvKyUXf6pWEWOnuKomk8aENbT6nTr7naRJb5L3J4zhE-5O_Yetw9aCTzy9vN8a22n0JHXeroAwTpLR_wsQwDPwN-K99JVUKwR-FvOkJhE7_wwbUXmjiacKjXrwQ0OWnhXigQRLfdHG2OyH6_It5dpBmBOyWx2X-tfQ6Wz-_2bKCALl487Amq56hhNJhbQuJFIR59RylVAWKmfeeno2qcTZgrI_mO3PJCCUxBn5hK81HJuOtZ4YmeDHPvLW8Tiv5KqfRMWJKhyFthB74FvUINiEn0jvbuLR3YuyTgpf22lohT4-mHq5FrEd3plGvj0fVI_zeGhAFBhQYMW-MAJo7oylTOMtSZ1JHHuvBPR6FvMTgaPTAum6Dsl-I4_O_OKgtgovefBgwh4TOm_vsJmjVYFRr0Eo3OqsfNw3OwSKnuv5I76thh6DN879UZiyJG_7lcz_L6d0g4fGCvdM45zgQp3U3l8fJN1MRYCx5mxJAYeVlnCpmqueuww";

  std::string key = read_from_file(RSA512_PRIV_KEY);
  ASSERT_TRUE (key.length());

  jwt::jwt_object obj{algorithm("RS512"), secret(key)};
  obj.add_claim("iss", "arun.muralidharan")
     .add_claim("aud", "all")
     .add_claim("exp", 1513862372)
     ;

  auto enc_str = obj.signature();
  EXPECT_EQ (enc_str, expected_str);


  key = read_from_file(RSA512_PUB_KEY);
  ASSERT_TRUE (key.length());

  auto dec_obj = jwt::decode(enc_str, algorithms({"none", "HS384", "RS512"}), verify(false), secret(key));
  EXPECT_EQ (dec_obj.header().algo(), jwt::algorithm::RS512);
}

TEST (RSAAlgo, NoSpecificAlgo)
{
  using namespace jwt::params;

  std::string key = read_from_file(RSA512_PRIV_KEY);
  ASSERT_TRUE (key.length());

  jwt::jwt_object obj{algorithm("RS512"), secret(key)};
  obj.add_claim("iss", "arun.muralidharan")
     .add_claim("aud", "all")
     .add_claim("exp", 1513862372)
     ;

  auto enc_str = obj.signature();
  key = read_from_file(RSA512_PUB_KEY);
  ASSERT_TRUE (key.length());

  EXPECT_THROW (jwt::decode(enc_str, algorithms({"none", "HS384", "RS384"}), verify(true), secret(key)),
                jwt::InvalidAlgorithmError);
}

