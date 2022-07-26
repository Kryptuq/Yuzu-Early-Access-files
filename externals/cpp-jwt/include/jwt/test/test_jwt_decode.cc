#include <iostream>
#include "jwt/jwt.hpp"

void basic_decode_test()
{
  // Create header
  jwt::jwt_header hdr;
  hdr = jwt::jwt_header{jwt::algorithm::HS256};

  // Create payload
  jwt::jwt_payload jp;
  jp.add_claim("sub", "1234567890");
  jp.add_claim("name", "John Doe");
  jp.add_claim("admin", true);

  jwt::jwt_signature sgn{"secret"};
  std::error_code ec{};
  auto res = sgn.encode(hdr, jp, ec);
  std::cout << res << std::endl;

  using namespace jwt::params;

  std::cout << "DECODE: \n";
  jwt::decode(res, algorithms({"none", "HS256"}), ec, verify(false), secret("secret"));
}

int main() {
  basic_decode_test();
  return 0;
}
