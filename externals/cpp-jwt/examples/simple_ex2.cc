#include <chrono>
#include <cassert>
#include <iostream>
#include "jwt/jwt.hpp"

int main() {
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret"), payload({{"user", "admin"}})};

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

  //Remove a claim using `remove_claim` API.
  //Most APIs have an overload which takes enum class type as well
  //It can be used interchangeably with strings.
  obj.remove_claim(jwt::registered_claims::expiration);
  assert (!obj.has_claim("exp"));

  //Using `add_claim` with extra features.
  //Check return status and overwrite
  assert (!obj.payload().add_claim("sub", "new test", false/*overwrite*/));

  // Overwrite an existing claim
  assert (obj.payload().add_claim("sub", "new test", true/*overwrite*/));

  assert (obj.payload().has_claim_with_value("sub", "new test"));

  return 0;
}
