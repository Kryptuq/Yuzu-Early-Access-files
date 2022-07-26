#include <iostream>
#include <string>
#if defined( CPP_JWT_USE_VENDORED_NLOHMANN_JSON)
#include "./json.hpp"
#else
#include "nlohmann/json.hpp"
#endif
using json = nlohmann::json;

void basic_json_test()
{
  json obj = json::object();
  obj["test"] = "value-test";
  obj["test-int"] = 42;

  std::string jstr = obj.dump(0);
  std::cout << jstr << std::endl;
}

int main() {
  basic_json_test();

  return 0;
}
