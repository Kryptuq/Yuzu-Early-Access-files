#include "gtest/gtest.h"
#include "jwt/jwt.hpp"

namespace {

struct Wrapper
{
    // The std::move here is required to resolve to the move ctor
    // rather than to the universal reference ctor.
    Wrapper(jwt::jwt_object&& obj) : object{std::move(obj)} {}
    jwt::jwt_object object;
};

} // END namespace

TEST (ObjectTest, MoveConstructor)
{
  using namespace jwt::params;

  jwt::jwt_object obj{algorithm("HS256"), secret("secret")};

  obj.add_claim("iss", "arun.muralidharan");

  auto wrapper = Wrapper{std::move(obj)};

  EXPECT_EQ(wrapper.object.header().algo(), jwt::algorithm::HS256);
  EXPECT_EQ(wrapper.object.secret(), "secret");
  EXPECT_TRUE(wrapper.object.payload().has_claim_with_value("iss", "arun.muralidharan"));
}

