#include <chrono>
#include <cassert>
#include <fstream>
#include <string>
#include <iostream>
#include "jwt/jwt.hpp"

/***
 * STEPS TO GENERATE RSA PRIVATE PUBLIC KEYPAIR.
 *
 * 1. openssl genrsa -out jwtRS256.key 1024
 * 2. openssl rsa -in jwtRS256.key -pubout -outform PEM -out jwtRS256.key.pub
 */

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
  } else {
    std::cerr << "FILE not FOUND!!" << std::endl;
  }

  is.close();
  return contents;
}

int main() {
  using namespace jwt::params;
  const std::string priv_key_path =  std::string{CERT_ROOT_DIR} + "/jwtRS256.key";
  const std::string pub_key_path = std::string{CERT_ROOT_DIR} + "/jwtRS256.key.pub";

  auto priv_key = read_from_file(priv_key_path);

  jwt::jwt_object obj{algorithm("RS256"), secret(priv_key), payload({{"user", "admin"}})};

  //Use add_claim API to add claim values which are
  // _not_ strings.
  // For eg: `iat` and `exp` claims below.
  // Other claims could have been added in the payload
  // function above as they are just stringy things.
  obj.add_claim("iss", "arun.muralidharan")
     .add_claim("sub", "test")
     .add_claim("id", "a-b-c-d-e-f-1-2-3")
     .add_claim("iat", 1513862371)
     .add_claim("exp", std::chrono::system_clock::now() + std::chrono::seconds{10})
     ;

  //Use `has_claim` to check if the claim exists or not
  assert (obj.has_claim("iss"));
  assert (obj.has_claim("exp"));

  //Use `has_claim_with_value` to check if the claim exists
  //with a specific value or not.
  assert (obj.payload().has_claim_with_value("id", "a-b-c-d-e-f-1-2-3"));
  assert (obj.payload().has_claim_with_value("iat", 1513862371));

  auto pub_key = read_from_file(pub_key_path);

  std::error_code ec{};
  auto sign = obj.signature(ec);
  if (ec) {
    std::cerr << ec.message() << std::endl;
    return 1;
  }

  auto dec_obj = jwt::decode(sign, algorithms({"RS256"}), verify(false), secret(pub_key));

  return 0;
}
