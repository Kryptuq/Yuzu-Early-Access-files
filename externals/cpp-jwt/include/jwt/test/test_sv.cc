#include <iostream>
#include <cassert>
#include <cstring>
#include <memory>
#include "jwt/string_view.hpp"

using string_view = jwt::basic_string_view<char>;

void basic_cons()
{
  // Default construction
  string_view sv{};
  assert (sv.length() == 0 && "Size must be zero for default constructor");

  // Construction from string literal
  string_view sv2{"Arun Muralidharan"};
  assert (sv2.length() == strlen("Arun Muralidharan") && "Lengths must match");

  const char* haystack = "some really big data with infinite objects....";

  // Construct using part of data
  string_view sv3{haystack, 4};
  assert (sv3.length() == 4 && "Partial construction is not ok");
  assert (sv3.to_string() == "some" && "Partial strings are not equal");

  return;
}

void iterator_test()
{
  string_view sv{"Arun Muralidharan"};
  for (auto c : sv) std::cout << c;
  std::cout << std::endl;
  return;
}

void str_operations()
{
  string_view sv{"Arun Muralidharan"};
  string_view tmp = sv;
  sv.remove_prefix(5);
  assert (sv.to_string() == "Muralidharan" && "Remove prefix failed");

  sv = tmp;
  sv.remove_suffix(strlen("Muralidharan"));
  assert (sv.to_string() == "Arun " && "Remove suffix failed");

  sv=tmp;
  {
    std::unique_ptr<char[]> dst{new char[32]};
    sv.copy(dst.get(), 6, 0);
    dst[6] = '\0';
    assert (strlen(dst.get()) == 6 && "Copy Failed-1");
    assert (std::string{dst.get()} == "Arun M" && "Copy Failed-2");

    sv.copy(dst.get(), 8, 4);
    dst[8] = '\0';
    assert (strlen(dst.get()) == 8 && "Middle copy failed-1");
    assert (std::string{dst.get()} == " Muralid" && "Middle copy failed-2");
  }

  {
    auto ss1 = sv.substr(0, 4);
    assert (ss1.to_string() == "Arun" && "Substr failed - 1");

    auto ss2 = sv.substr(1, 3);
    assert (ss2.to_string() == "run" && "Substr failed - 2");

    auto ss3 = sv.substr(0);
    assert (ss3.length() == sv.length() && "Substr failed - 3");
  }

  return;
}

void find_oper()
{
  string_view sv{"Arun Muralidharan"};
  auto pos = sv.find("Arun", 0, 4);
  assert (pos == 0 && "Arun not found in sv");

  pos = sv.find("arun", 0, 4);
  assert (pos == string_view::npos && "arun is not there in sv");

  sv = "This has a, in it.";
  pos = sv.find_first_of(",", 0, 1);
  assert (pos != string_view::npos);
  assert (pos == 10 && "Comma not found at correct place");

  pos = sv.find_first_of(",", 10, 1);
  assert (pos != string_view::npos);
  assert (pos == 10 && "Comma not found at correct place");

  pos = sv.find_first_of(":", 10, 1);
  assert (pos == string_view::npos);

  pos = sv.find_last_of(",", 5, 1);
  assert (pos == string_view::npos);

  pos = sv.find_last_of(",", sv.length() - 1, 1);
  assert (pos != string_view::npos);
  assert (pos == 10 && "Comma not found at correct place");

  pos = sv.find_first_of(".", 0, 1);
  assert (pos == sv.length() - 1 && "Dot not found at correct place");

  pos = sv.find_last_of(".", sv.length() - 2, 1);
  assert (pos == string_view::npos);

  pos = sv.find_last_of(".", sv.length() - 1, 1);
  assert (pos == sv.length() - 1);

  sv = "Some string :<> with some ??? pattern --**";

  pos = sv.rfind("???", sv.length() - 1, 3);
  assert (pos != string_view::npos && "??? not found");
  assert (pos == 26 && "??? not found at the correct place");

  sv = "ATCGTTCACGRRRTCGGGGACGTC";

  pos = sv.find_first_not_of("ATCG");
  assert (pos != string_view::npos);
  assert (pos == 10);

  return;
}

void conversions()
{
  auto c2sv = [](int num) -> string_view {
    switch (num) {
    case 1: return "one";
    case 2: return "two";
    case 3: return "three";
    default: return "many";
    };
  };

  auto res = c2sv(2);
  assert (res.to_string() == "two");

  auto s2sv = [](std::string s) {
    return s;
  };

  s2sv(static_cast<std::string>(res));
}

void comparisons()
{
  string_view s1{"Apple"};
  string_view s2{"Orange"};

  assert (s1 != s2 && "Two string views are not equal");
  assert (s2 > s1  && "Orange is lexicographically bigger than Apple");

  s2 = "Apples";
  assert (s2 > s1 && "Because Apples is plural");
}

int main() {
  basic_cons();
  iterator_test();
  str_operations();
  find_oper();
  conversions();
  comparisons();
  return 0;
};
