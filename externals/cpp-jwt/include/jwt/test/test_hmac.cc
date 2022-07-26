#include <iostream>
#include "jwt/algorithm.hpp"

void basic_hmac_test()
{
  jwt::string_view sv = "secret" ;
  jwt::string_view d = "Some random data string";
  auto res = jwt::HMACSign<jwt::algo::HS256>::sign(sv, d);

  std::cout << res.first << std::endl;
}

int main() {
  basic_hmac_test();
  return 0;
}
