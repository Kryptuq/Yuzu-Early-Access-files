#include <iostream>
#include <string>
#include <cassert>
#include "jwt/base64.hpp"

void base64_test_encode()
{
  std::string input = "ArunMu";
  std::string output = jwt::base64_encode(input.c_str(), input.length());
  assert (output == "QXJ1bk11");

  input = "Something really strange!!";
  output = jwt::base64_encode(input.c_str(), input.length());
  assert (output == "U29tZXRoaW5nIHJlYWxseSBzdHJhbmdlISE=");

  input = "Do you want to know something more stranger ????";
  output = jwt::base64_encode(input.c_str(), input.length());
  assert (output == "RG8geW91IHdhbnQgdG8ga25vdyBzb21ldGhpbmcgbW9yZSBzdHJhbmdlciA/Pz8/");

  input = R"({"a" : "b", "c" : [1,2,3,4,5]})";
  output = jwt::base64_encode(input.c_str(), input.length());
  assert (output == "eyJhIiA6ICJiIiwgImMiIDogWzEsMiwzLDQsNV19");
}

void base64_test_decode()
{
  std::string input = "QXJ1bk11";
  std::string output = jwt::base64_decode(input.c_str(), input.length());
  assert (output == "ArunMu");

  input = "U29tZXRoaW5nIHJlYWxseSBzdHJhbmdlISE=";
  output = jwt::base64_decode(input.c_str(), input.length());
  assert (output == "Something really strange!!");

  input = "RG8geW91IHdhbnQgdG8ga25vdyBzb21ldGhpbmcgbW9yZSBzdHJhbmdlciA/Pz8/";
  output = jwt::base64_decode(input.c_str(), input.length());
  assert (output == "Do you want to know something more stranger ????");

  input = "eyJhIiA6ICJiIiwgImMiIDogWzEsMiwzLDQsNV19";
  output = jwt::base64_decode(input.c_str(), input.length());
  assert (output == R"({"a" : "b", "c" : [1,2,3,4,5]})");
}

int main() {
  base64_test_encode();
  base64_test_decode();
  return 0;
}
