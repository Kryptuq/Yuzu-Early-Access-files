#include <iostream>
#include "jwt/jwt.hpp"

void basic_sign_test()
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
}

int main() {
  basic_sign_test();
  return 0;
}
