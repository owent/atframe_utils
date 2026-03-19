// Copyright 2026 atframework

#include <string>
#include <vector>

#include "string/tquerystring.h"

#include "frame/test_macros.h"

// ======== URI encoding/decoding tests ========

CASE_TEST(tquerystring, encode_uri_basic) {
  // Characters that should NOT be encoded by encode_uri: A-Z a-z 0-9 - _ . ~ ! * ' ( ) ; / ? : @ & = + $ , #
  std::string safe = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  std::string encoded = atfw::util::uri::encode_uri(safe.c_str(), safe.size());
  CASE_EXPECT_EQ(safe, encoded);

  // Space should be encoded
  std::string with_space = "hello world";
  encoded = atfw::util::uri::encode_uri(with_space.c_str(), with_space.size());
  CASE_EXPECT_TRUE(encoded.find("%20") != std::string::npos);
}

CASE_TEST(tquerystring, decode_uri_basic) {
  std::string encoded = "hello%20world";
  std::string decoded = atfw::util::uri::decode_uri(encoded.c_str(), encoded.size());
  CASE_EXPECT_EQ("hello world", decoded);
}

CASE_TEST(tquerystring, encode_decode_uri_component_roundtrip) {
  std::string original = "key=value&foo=bar baz";
  std::string encoded = atfw::util::uri::encode_uri_component(original.c_str(), original.size());
  std::string decoded = atfw::util::uri::decode_uri_component(encoded.c_str(), encoded.size());
  CASE_EXPECT_EQ(original, decoded);
}

CASE_TEST(tquerystring, encode_uri_component_special_chars) {
  // encode_uri_component should encode: = & + ? / : @ # ; , $
  std::string input = "a=b&c=d";
  std::string encoded = atfw::util::uri::encode_uri_component(input.c_str(), input.size());
  CASE_EXPECT_TRUE(encoded.find('=') == std::string::npos);
  CASE_EXPECT_TRUE(encoded.find('&') == std::string::npos);
}

CASE_TEST(tquerystring, decode_uri_component_malformed) {
  // Malformed percent encoding - trailing %
  std::string input = "abc%";
  std::string decoded = atfw::util::uri::decode_uri_component(input.c_str(), input.size());
  // Should not crash, result may vary
  CASE_EXPECT_FALSE(decoded.empty());

  // Single hex digit after %
  std::string input2 = "abc%2";
  decoded = atfw::util::uri::decode_uri_component(input2.c_str(), input2.size());
  CASE_EXPECT_FALSE(decoded.empty());
}

CASE_TEST(tquerystring, raw_encode_decode_url_roundtrip) {
  std::string original = "hello world+test";
  std::string encoded = atfw::util::uri::raw_encode_url(original.c_str(), original.size());
  std::string decoded = atfw::util::uri::raw_decode_url(encoded.c_str(), encoded.size());
  CASE_EXPECT_EQ(original, decoded);
}

CASE_TEST(tquerystring, encode_decode_url_roundtrip) {
  // encode_url encodes space as + (application/x-www-form-urlencoded)
  std::string original = "hello world";
  std::string encoded = atfw::util::uri::encode_url(original.c_str(), original.size());
  CASE_EXPECT_TRUE(encoded.find('+') != std::string::npos);

  std::string decoded = atfw::util::uri::decode_url(encoded.c_str(), encoded.size());
  CASE_EXPECT_EQ(original, decoded);
}

CASE_TEST(tquerystring, encode_url_special) {
  std::string input = "a&b=c d";
  std::string encoded = atfw::util::uri::encode_url(input.c_str(), input.size());
  // & and = should be encoded
  CASE_EXPECT_TRUE(encoded.find('&') == std::string::npos || encoded.find("%26") != std::string::npos);
}

CASE_TEST(tquerystring, encode_uri_empty) {
  std::string encoded = atfw::util::uri::encode_uri("", 0);
  CASE_EXPECT_TRUE(encoded.empty());

  // Use null-terminated string with default sz=0
  encoded = atfw::util::uri::encode_uri("");
  CASE_EXPECT_TRUE(encoded.empty());
}

CASE_TEST(tquerystring, decode_uri_empty) {
  std::string decoded = atfw::util::uri::decode_uri("", 0);
  CASE_EXPECT_TRUE(decoded.empty());
}

// ======== item_string tests ========

CASE_TEST(tquerystring, item_string_basic) {
  auto str = atfw::util::types::item_string::create("hello");
  CASE_EXPECT_FALSE(str->empty());
  CASE_EXPECT_EQ(5, static_cast<int>(str->size()));
  CASE_EXPECT_EQ(atfw::util::types::ITEM_TYPE_STRING, str->type());
  CASE_EXPECT_EQ("hello", str->data());
  CASE_EXPECT_EQ("hello", str->to_string());
}

CASE_TEST(tquerystring, item_string_empty) {
  auto str = atfw::util::types::item_string::create();
  CASE_EXPECT_TRUE(str->empty());
  CASE_EXPECT_EQ(0, static_cast<int>(str->size()));
}

CASE_TEST(tquerystring, item_string_set_get) {
  auto str = atfw::util::types::item_string::create();
  str->set("world");
  CASE_EXPECT_EQ("world", str->get());

  *str = "updated";
  CASE_EXPECT_EQ("updated", str->data());
}

CASE_TEST(tquerystring, item_string_encode) {
  auto str = atfw::util::types::item_string::create("value");
  std::string output;
  CASE_EXPECT_TRUE(str->encode(output, "key"));
  CASE_EXPECT_FALSE(output.empty());
  CASE_EXPECT_TRUE(output.find("key") != std::string::npos);
  CASE_EXPECT_TRUE(output.find("value") != std::string::npos);
}

// ======== item_array tests ========

CASE_TEST(tquerystring, item_array_basic) {
  auto arr = atfw::util::types::item_array::create();
  CASE_EXPECT_TRUE(arr->empty());
  CASE_EXPECT_EQ(0, static_cast<int>(arr->size()));
  CASE_EXPECT_EQ(atfw::util::types::ITEM_TYPE_ARRAY, arr->type());

  arr->append("first");
  arr->append("second");
  CASE_EXPECT_FALSE(arr->empty());
  CASE_EXPECT_EQ(2, static_cast<int>(arr->size()));
}

CASE_TEST(tquerystring, item_array_get_set) {
  auto arr = atfw::util::types::item_array::create();
  arr->append("a");
  arr->append("b");
  arr->append("c");

  CASE_EXPECT_EQ("a", arr->get_string(0));
  CASE_EXPECT_EQ("b", arr->get_string(1));
  CASE_EXPECT_EQ("c", arr->get_string(2));

  arr->set(1, "B");
  CASE_EXPECT_EQ("B", arr->get_string(1));
}

CASE_TEST(tquerystring, item_array_pop_clear) {
  auto arr = atfw::util::types::item_array::create();
  arr->append("x");
  arr->append("y");
  CASE_EXPECT_EQ(2, static_cast<int>(arr->size()));

  arr->pop_back();
  CASE_EXPECT_EQ(1, static_cast<int>(arr->size()));

  arr->clear();
  CASE_EXPECT_TRUE(arr->empty());
}

CASE_TEST(tquerystring, item_array_encode) {
  auto arr = atfw::util::types::item_array::create();
  arr->append("val1");
  arr->append("val2");

  std::string output;
  CASE_EXPECT_TRUE(arr->encode(output, "arr"));
  CASE_EXPECT_FALSE(output.empty());
}

CASE_TEST(tquerystring, item_array_to_string) {
  auto arr = atfw::util::types::item_array::create();
  arr->append("hello");
  arr->append("world");

  std::string result = arr->to_string("items");
  CASE_EXPECT_FALSE(result.empty());
}

// ======== item_object tests ========

CASE_TEST(tquerystring, item_object_basic) {
  auto obj = atfw::util::types::item_object::create();
  CASE_EXPECT_TRUE(obj->empty());
  CASE_EXPECT_EQ(0, static_cast<int>(obj->size()));

  obj->set("key1", "value1");
  obj->set("key2", "value2");
  CASE_EXPECT_FALSE(obj->empty());
  CASE_EXPECT_EQ(2, static_cast<int>(obj->size()));
}

CASE_TEST(tquerystring, item_object_get_set) {
  auto obj = atfw::util::types::item_object::create();
  obj->set("name", "test");
  CASE_EXPECT_EQ("test", obj->get_string("name"));

  obj->set("name", "updated");
  CASE_EXPECT_EQ("updated", obj->get_string("name"));
}

CASE_TEST(tquerystring, item_object_remove) {
  auto obj = atfw::util::types::item_object::create();
  obj->set("a", "1");
  obj->set("b", "2");
  CASE_EXPECT_EQ(2, static_cast<int>(obj->size()));

  obj->remove("a");
  CASE_EXPECT_EQ(1, static_cast<int>(obj->size()));
  CASE_EXPECT_TRUE(obj->get_string("a").empty());
}

CASE_TEST(tquerystring, item_object_keys) {
  auto obj = atfw::util::types::item_object::create();
  obj->set("alpha", "1");
  obj->set("beta", "2");

  auto keys = obj->keys();
  CASE_EXPECT_EQ(2, static_cast<int>(keys.size()));
}

CASE_TEST(tquerystring, item_object_clear) {
  auto obj = atfw::util::types::item_object::create();
  obj->set("k", "v");
  obj->clear();
  CASE_EXPECT_TRUE(obj->empty());
}

CASE_TEST(tquerystring, item_object_encode) {
  auto obj = atfw::util::types::item_object::create();
  obj->set("key", "value");

  std::string output;
  CASE_EXPECT_TRUE(obj->encode(output, "obj"));
  CASE_EXPECT_FALSE(output.empty());
}

// ======== tquerystring decode/encode tests ========

CASE_TEST(tquerystring, decode_bracket_params) {
  // Library requires bracket notation: key[subkey]=value
  atfw::util::tquerystring qs;
  qs.decode("data[a]=1&data[b]=2&data[c]=3");

  auto data = qs["data"];
  CASE_EXPECT_TRUE(nullptr != data);
  if (data) {
    auto data_obj = std::static_pointer_cast<atfw::util::types::item_object>(data);
    if (data_obj) {
      CASE_EXPECT_EQ("1", data_obj->get_string("a"));
      CASE_EXPECT_EQ("2", data_obj->get_string("b"));
      CASE_EXPECT_EQ("3", data_obj->get_string("c"));
    }
  }
}

CASE_TEST(tquerystring, encode_output) {
  // encode() produces key=value& format from stored data
  atfw::util::tquerystring qs;
  qs.set("name", "hello");
  qs.set("value", "world");

  std::string encoded;
  qs.encode(encoded);
  CASE_EXPECT_FALSE(encoded.empty());
  // Output should contain the keys and values
  CASE_EXPECT_TRUE(encoded.find("name") != std::string::npos);
  CASE_EXPECT_TRUE(encoded.find("hello") != std::string::npos);
}

CASE_TEST(tquerystring, decode_array_indexed) {
  // Library requires explicit index in brackets: arr[0]=val
  atfw::util::tquerystring qs;
  qs.decode("arr[0]=first&arr[1]=second&arr[2]=third");

  auto item = qs["arr"];
  CASE_EXPECT_TRUE(nullptr != item);
  if (item) {
    auto arr_obj = std::static_pointer_cast<atfw::util::types::item_object>(item);
    if (arr_obj) {
      CASE_EXPECT_EQ("first", arr_obj->get_string("0"));
      CASE_EXPECT_EQ("second", arr_obj->get_string("1"));
      CASE_EXPECT_EQ("third", arr_obj->get_string("2"));
    }
  }
}

CASE_TEST(tquerystring, decode_nested_object) {
  atfw::util::tquerystring qs;
  qs.decode("user[name]=test&user[age]=25");

  auto user = qs["user"];
  CASE_EXPECT_TRUE(nullptr != user);
  if (user) {
    auto user_obj = std::static_pointer_cast<atfw::util::types::item_object>(user);
    CASE_EXPECT_TRUE(nullptr != user_obj);
    if (user_obj) {
      CASE_EXPECT_EQ("test", user_obj->get_string("name"));
      CASE_EXPECT_EQ("25", user_obj->get_string("age"));
    }
  }
}

CASE_TEST(tquerystring, decode_url_encoded_bracket_values) {
  // URL-encoded values with bracket notation
  atfw::util::tquerystring qs;
  qs.decode("data[key]=hello%20world&data[foo]=bar");

  auto data = qs["data"];
  CASE_EXPECT_TRUE(nullptr != data);
  if (data) {
    auto data_obj = std::static_pointer_cast<atfw::util::types::item_object>(data);
    if (data_obj) {
      CASE_EXPECT_EQ("hello world", data_obj->get_string("key"));
      CASE_EXPECT_EQ("bar", data_obj->get_string("foo"));
    }
  }
}

CASE_TEST(tquerystring, empty_querystring) {
  atfw::util::tquerystring qs;
  CASE_EXPECT_TRUE(qs.empty());
  CASE_EXPECT_EQ(0, static_cast<int>(qs.size()));
  CASE_EXPECT_EQ(atfw::util::types::ITEM_TYPE_QUERYSTRING, qs.type());
}

CASE_TEST(tquerystring, set_spliter) {
  atfw::util::tquerystring qs;
  qs.set_spliter(";");
  qs.decode("d[a]=1;d[b]=2;d[c]=3");

  auto d = qs["d"];
  CASE_EXPECT_TRUE(nullptr != d);
  if (d) {
    auto d_obj = std::static_pointer_cast<atfw::util::types::item_object>(d);
    if (d_obj) {
      CASE_EXPECT_EQ("1", d_obj->get_string("a"));
      CASE_EXPECT_EQ("2", d_obj->get_string("b"));
      CASE_EXPECT_EQ("3", d_obj->get_string("c"));
    }
  }
}

CASE_TEST(tquerystring, create_with_spliter) {
  auto qs = atfw::util::tquerystring::create(";");
  qs->decode("p[x]=10;p[y]=20");

  auto p = (*qs)["p"];
  CASE_EXPECT_TRUE(nullptr != p);
  if (p) {
    auto p_obj = std::static_pointer_cast<atfw::util::types::item_object>(p);
    if (p_obj) {
      CASE_EXPECT_EQ("10", p_obj->get_string("x"));
      CASE_EXPECT_EQ("20", p_obj->get_string("y"));
    }
  }
}

CASE_TEST(tquerystring, create_helpers) {
  atfw::util::tquerystring qs;
  auto str = qs.create_string("test");
  CASE_EXPECT_FALSE(str->empty());

  auto arr = qs.create_array();
  CASE_EXPECT_TRUE(arr->empty());

  auto obj = qs.create_object();
  CASE_EXPECT_TRUE(obj->empty());
}

CASE_TEST(tquerystring, to_string_output) {
  atfw::util::tquerystring qs;
  qs.set("key", "value");
  std::string result = qs.to_string();
  CASE_EXPECT_FALSE(result.empty());
}

CASE_TEST(tquerystring, operator_bracket) {
  atfw::util::tquerystring qs;
  qs.set("test", "123");
  auto item = qs["test"];
  CASE_EXPECT_TRUE(nullptr != item);

  auto missing = qs["nonexistent"];
  CASE_EXPECT_TRUE(nullptr == missing);
}

CASE_TEST(tquerystring, decode_deep_nested) {
  atfw::util::tquerystring qs;
  qs.decode("a[b][c]=deep");

  auto a = qs["a"];
  CASE_EXPECT_TRUE(nullptr != a);
  if (a) {
    auto a_obj = std::static_pointer_cast<atfw::util::types::item_object>(a);
    if (a_obj) {
      auto b = a_obj->get("b");
      CASE_EXPECT_TRUE(nullptr != b);
    }
  }
}

CASE_TEST(tquerystring, item_object_data_access) {
  auto obj = atfw::util::types::item_object::create();
  obj->set("k1", "v1");
  obj->set("k2", "v2");

  const auto &data = obj->data();
  CASE_EXPECT_EQ(2, static_cast<int>(data.size()));
  CASE_EXPECT_TRUE(data.find("k1") != data.end());
  CASE_EXPECT_TRUE(data.find("k2") != data.end());
}

CASE_TEST(tquerystring, item_array_get_shared_ptr) {
  auto arr = atfw::util::types::item_array::create();
  arr->append("test");
  auto item = arr->get(0);
  CASE_EXPECT_TRUE(nullptr != item);
  if (item) {
    CASE_EXPECT_EQ(atfw::util::types::ITEM_TYPE_STRING, item->type());
  }
}

CASE_TEST(tquerystring, item_array_set_with_item) {
  auto arr = atfw::util::types::item_array::create();
  arr->append("placeholder");

  auto str_item = atfw::util::types::item_string::create("replaced");
  arr->set(0, str_item);

  CASE_EXPECT_EQ("replaced", arr->get_string(0));
}

CASE_TEST(tquerystring, item_object_get_shared_ptr) {
  auto obj = atfw::util::types::item_object::create();
  obj->set("key", "value");

  auto item = obj->get("key");
  CASE_EXPECT_TRUE(nullptr != item);

  auto missing = obj->get("missing");
  CASE_EXPECT_TRUE(nullptr == missing);
}

CASE_TEST(tquerystring, item_object_set_with_item) {
  auto obj = atfw::util::types::item_object::create();

  auto str_item = atfw::util::types::item_string::create("custom");
  obj->set("key", str_item);

  CASE_EXPECT_EQ("custom", obj->get_string("key"));
}

CASE_TEST(tquerystring, item_object_data) {
  auto obj = atfw::util::types::item_object::create();
  obj->set("a", "1");
  obj->set("b", "2");

  auto &data = obj->data();
  CASE_EXPECT_EQ(2, static_cast<int>(data.size()));
}

CASE_TEST(tquerystring, item_string_parse) {
  auto str = atfw::util::types::item_string::create();
  std::vector<std::string> keys = {"key"};
  CASE_EXPECT_TRUE(str->parse(keys, 0, "parsed_value"));
  CASE_EXPECT_EQ("parsed_value", str->data());
}
