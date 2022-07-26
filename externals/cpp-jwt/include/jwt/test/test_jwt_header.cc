#include <iostream>
#include "jwt/jwt.hpp"

void test_basic_header()
{
  jwt::jwt_header hdr;
  hdr = jwt::jwt_header{jwt::algorithm::HS256};
  std::string jstr = to_json_str(hdr);
  std::cout << jstr << std::endl;

  std::string enc_str = hdr.base64_encode();
  std::cout << "Base64: " << enc_str << std::endl;
  std::cout << "Decoded: " << hdr.base64_decode(enc_str) << std::endl;
}

int main() {
  test_basic_header();
  return 0;
}
