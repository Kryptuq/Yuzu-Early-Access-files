#include <iostream>
#include "jwt/jwt.hpp"

void basic_payload_test()
{
  jwt::jwt_payload jp;
  jp.add_claim("iss", "myself");
  jp.add_claim("exp", 1234567);
  jp.add_claim("Exp", 1234567, true);

  auto jstr = jwt::to_json_str(jp);
  std::cout << jstr << std::endl;

  auto enc = jp.base64_encode();
  std::cout << "Base64 enc: " << enc << std::endl;

  auto dec = jp.base64_decode(enc);
  std::cout << "Base64 dec: " << dec << std::endl;
  std::cout << "Base64 dec: " << jstr << std::endl;

  assert (jstr == dec && "Encoded and decoded messages do not match");
  assert (jp.has_claim("exp") && "Claim exp must exist");
  assert (jp.has_claim("Exp") && "Claim Exp must exist");

  assert (!jp.has_claim("aud") && "Claim aud does not exist");
  assert (jp.has_claim_with_value("exp", 1234567) && "Claim exp with value 1234567 does not exist");

  return;
}

int main() {
  basic_payload_test();
  return 0;
}
