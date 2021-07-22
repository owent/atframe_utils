#include <time.h>
#include <algorithm>

#include <config/compiler_features.h>

#include "frame/test_macros.h"
#include "random/random_generator.h"
#include "random/uuid_generator.h"

#if defined(LIBATFRAME_UTILS_ENABLE_UNORDERED_MAP_SET) && LIBATFRAME_UTILS_ENABLE_UNORDERED_MAP_SET
#  include <unordered_set>
#else
#  include <set>
#endif

template <typename TOS, typename TVEC>
void print_vec(const TOS &os, const std::vector<TVEC> &vec) {
  os << "[ ";
  for (size_t i = 0; i < vec.size(); ++i) {
    if (i > 0) {
      os << ", ";
    }

    os << vec[i];
  }

  os << " ]" << std::endl;
}

CASE_TEST(random_test, random_gen_mt19937) {
  util::random::mt19937 gen1;
  gen1.init_seed(123);

  uint32_t a1 = gen1.random();
  uint32_t a2 = gen1.random();
  uint32_t a3 = gen1();

  CASE_EXPECT_EQ(static_cast<uint32_t>(2991312382), a1);
  CASE_EXPECT_EQ(static_cast<uint32_t>(3062119789), a2);
  CASE_EXPECT_EQ(static_cast<uint32_t>(1228959102), a3);

  std::vector<uint32_t> shuffle_arr;
  for (int i = 0; i < 10; ++i) {
    shuffle_arr.push_back(gen1.random());
  }

  gen1.shuffle(shuffle_arr.begin(), shuffle_arr.end());
  print_vec(CASE_MSG_INFO() << "random_shuffle => ", shuffle_arr);

  // Test load/dump
  {
    std::vector<unsigned char> buffer;
    uint64_t first_random[256] = {0};
    bool check_result = false;
    CASE_EXPECT_GT(gen1.block_size(), 0);
    buffer.resize(gen1.block_size());
    CASE_EXPECT_FALSE(gen1.dump(nullptr, buffer.size()));
    CASE_EXPECT_FALSE(gen1.dump(&buffer[0], buffer.size() - 1));
    CASE_EXPECT_TRUE(gen1.dump(&buffer[0], buffer.size()));

    for (int i = 0; i < 256; ++i) {
      first_random[i] = static_cast<uint64_t>(gen1.random());
      if (0 != first_random[i]) {
        check_result = true;
      }
    }

    CASE_EXPECT_FALSE(gen1.load(nullptr, buffer.size()));
    CASE_EXPECT_FALSE(gen1.load(&buffer[0], buffer.size() - 1));
    CASE_EXPECT_TRUE(gen1.load(&buffer[0], buffer.size()));
    CASE_EXPECT_TRUE(check_result);

    for (int i = 0; i < 256; ++i) {
      uint64_t second_random = static_cast<uint64_t>(gen1.random());
      CASE_EXPECT_EQ(first_random[i], second_random);
      if (first_random[i] != second_random) {
        break;
      }
    }
  }
}

CASE_TEST(random_test, random_gen_mt19937_64) {
  util::random::mt19937_64 gen1;
  gen1.init_seed(321);

  uint64_t a1 = gen1.random();
  uint64_t a2 = gen1.random();
  uint64_t a3 = gen1();

  CASE_EXPECT_EQ(10254371118423419891ULL, a1);
  CASE_EXPECT_EQ(8078970281289081ULL, a2);
  CASE_EXPECT_EQ(13774509987902109069ULL, a3);

  std::vector<uint64_t> shuffle_arr;
  for (int i = 0; i < 10; ++i) {
    shuffle_arr.push_back(gen1.random());
  }
  gen1.shuffle(shuffle_arr.begin(), shuffle_arr.end());
  print_vec(CASE_MSG_INFO() << "random_shuffle => ", shuffle_arr);

  // Test load/dump
  {
    std::vector<unsigned char> buffer;
    uint64_t first_random[256] = {0};
    bool check_result = false;
    CASE_EXPECT_GT(gen1.block_size(), 0);
    buffer.resize(gen1.block_size());
    CASE_EXPECT_FALSE(gen1.dump(nullptr, buffer.size()));
    CASE_EXPECT_FALSE(gen1.dump(&buffer[0], buffer.size() - 1));
    CASE_EXPECT_TRUE(gen1.dump(&buffer[0], buffer.size()));

    for (int i = 0; i < 256; ++i) {
      first_random[i] = static_cast<uint64_t>(gen1.random());
      if (0 != first_random[i]) {
        check_result = true;
      }
    }

    CASE_EXPECT_FALSE(gen1.load(nullptr, buffer.size()));
    CASE_EXPECT_FALSE(gen1.load(&buffer[0], buffer.size() - 1));
    CASE_EXPECT_TRUE(gen1.load(&buffer[0], buffer.size()));
    CASE_EXPECT_TRUE(check_result);

    for (int i = 0; i < 256; ++i) {
      uint64_t second_random = static_cast<uint64_t>(gen1.random());
      CASE_EXPECT_EQ(first_random[i], second_random);
      if (first_random[i] != second_random) {
        break;
      }
    }
  }
}

CASE_TEST(random_test, random_gen_64_mt11213B) {
  util::random::mt11213b gen1;
  gen1.init_seed(789);

  uint64_t a1 = gen1.random();
  uint64_t a2 = gen1.random();
  uint64_t a3 = gen1();

  CASE_EXPECT_EQ(static_cast<uint32_t>(3740219552), a1);
  CASE_EXPECT_EQ(static_cast<uint32_t>(740436508), a2);
  CASE_EXPECT_EQ(static_cast<uint32_t>(649207690), a3);

  std::vector<uint64_t> shuffle_arr;
  for (int i = 0; i < 10; ++i) {
    shuffle_arr.push_back(gen1.random());
  }
  gen1.shuffle(shuffle_arr.begin(), shuffle_arr.end());
  print_vec(CASE_MSG_INFO() << "random_shuffle => ", shuffle_arr);

  // Test load/dump
  {
    std::vector<unsigned char> buffer;
    uint64_t first_random[256] = {0};
    bool check_result = false;
    CASE_EXPECT_GT(gen1.block_size(), 0);
    buffer.resize(gen1.block_size());
    CASE_EXPECT_FALSE(gen1.dump(nullptr, buffer.size()));
    CASE_EXPECT_FALSE(gen1.dump(&buffer[0], buffer.size() - 1));
    CASE_EXPECT_TRUE(gen1.dump(&buffer[0], buffer.size()));

    for (int i = 0; i < 256; ++i) {
      first_random[i] = static_cast<uint64_t>(gen1.random());
      if (0 != first_random[i]) {
        check_result = true;
      }
    }

    CASE_EXPECT_FALSE(gen1.load(nullptr, buffer.size()));
    CASE_EXPECT_FALSE(gen1.load(&buffer[0], buffer.size() - 1));
    CASE_EXPECT_TRUE(gen1.load(&buffer[0], buffer.size()));
    CASE_EXPECT_TRUE(check_result);

    for (int i = 0; i < 256; ++i) {
      uint64_t second_random = static_cast<uint64_t>(gen1.random());
      CASE_EXPECT_EQ(first_random[i], second_random);
      if (first_random[i] != second_random) {
        break;
      }
    }
  }
}

CASE_TEST(random_test, random_gen_taus88) {
  util::random::taus88 gen1;
  gen1.init_seed(321);

  uint32_t a1 = gen1.random();
  uint32_t a2 = gen1.random();
  uint32_t a3 = gen1();

  CASE_EXPECT_EQ(static_cast<uint32_t>(43258884), a1);
  CASE_EXPECT_EQ(static_cast<uint32_t>(1073987586), a2);
  CASE_EXPECT_EQ(static_cast<uint32_t>(2769562570), a3);

  std::vector<uint32_t> shuffle_arr;
  for (int i = 0; i < 10; ++i) {
    shuffle_arr.push_back(gen1.random());
  }
  gen1.shuffle(shuffle_arr.begin(), shuffle_arr.end());
  print_vec(CASE_MSG_INFO() << "random_shuffle => ", shuffle_arr);

  // Test load/dump
  {
    std::vector<unsigned char> buffer;
    uint64_t first_random[256] = {0};
    bool check_result = false;
    CASE_EXPECT_GT(gen1.block_size(), 0);
    buffer.resize(gen1.block_size());
    CASE_EXPECT_FALSE(gen1.dump(nullptr, buffer.size()));
    CASE_EXPECT_FALSE(gen1.dump(&buffer[0], buffer.size() - 1));
    CASE_EXPECT_TRUE(gen1.dump(&buffer[0], buffer.size()));

    for (int i = 0; i < 256; ++i) {
      first_random[i] = static_cast<uint64_t>(gen1.random());
      if (0 != first_random[i]) {
        check_result = true;
      }
    }

    CASE_EXPECT_FALSE(gen1.load(nullptr, buffer.size()));
    CASE_EXPECT_FALSE(gen1.load(&buffer[0], buffer.size() - 1));
    CASE_EXPECT_TRUE(gen1.load(&buffer[0], buffer.size()));
    CASE_EXPECT_TRUE(check_result);

    for (int i = 0; i < 256; ++i) {
      uint64_t second_random = static_cast<uint64_t>(gen1.random());
      CASE_EXPECT_EQ(first_random[i], second_random);
      if (first_random[i] != second_random) {
        break;
      }
    }
  }
}

CASE_TEST(random_test, xoroshiro128_starstar) {
  util::random::xoroshiro128_starstar gen1;
  gen1.init_seed(0x12345678);

  uint32_t checked1[] = {0x6baaab3a, 0x9385fdbb, 0x99ae85fe, 0x88e2358c, 0x6592b0ce,
                         0x20eadc84, 0x6a8d80b7, 0xa676abb7, 0x726ef19a, 0xd4e33183};

  for (size_t i = 0; i < sizeof(checked1) / sizeof(checked1[0]); ++i) {
    CASE_EXPECT_EQ(gen1.random(), checked1[i]);
  }

  gen1.get_core().jump();
  uint32_t checked2[] = {0xb40ef1a3, 0xeaf5f926, 0xdaedd531, 0xd75a0342, 0xf5cd493d,
                         0x4081f259, 0x92a08a09, 0x8e7856ff, 0x3eecdb2,  0x34cd820c};

  for (size_t i = 0; i < sizeof(checked2) / sizeof(checked2[0]); ++i) {
    CASE_EXPECT_EQ(gen1.random(), checked2[i]);
  }

  uint32_t arr_seeds[] = {0x01, 0x02, 0x03, 0x04};
  uint32_t *ptr_seeds = &arr_seeds[0];
  gen1.init_seed(ptr_seeds, ptr_seeds + 4);

  uint32_t checked3[] = {0x483919e4, 0xa90f4ce8, 0xe55d1499, 0x17c2185e, 0xdb11c3f7,
                         0x63557f98, 0xc7ad6874, 0x4a4a45ca, 0x74dec471, 0xf7e82f2a};

  for (size_t i = 0; i < sizeof(checked3) / sizeof(checked3[0]); ++i) {
    CASE_EXPECT_EQ(gen1.random(), checked3[i]);
  }

  gen1.get_core().jump();
  uint32_t checked4[] = {0xf30187c0, 0xe533ff68, 0x628920df, 0xa998b9e7, 0xcac83081,
                         0xe23e175b, 0x8aef7f7a, 0x43e56ba2, 0x59dc108b, 0x35b5536c};

  for (size_t i = 0; i < sizeof(checked4) / sizeof(checked4[0]); ++i) {
    CASE_EXPECT_EQ(gen1.random(), checked4[i]);
  }

  std::vector<uint32_t> shuffle_arr;
  for (int i = 0; i < 10; ++i) {
    shuffle_arr.push_back(gen1.random());
  }
  gen1.shuffle(shuffle_arr.begin(), shuffle_arr.end());
  print_vec(CASE_MSG_INFO() << "random_shuffle => ", shuffle_arr);

  // Test load/dump
  {
    std::vector<unsigned char> buffer;
    uint64_t first_random[256] = {0};
    bool check_result = false;
    CASE_EXPECT_GT(gen1.block_size(), 0);
    buffer.resize(gen1.block_size());
    CASE_EXPECT_FALSE(gen1.dump(nullptr, buffer.size()));
    CASE_EXPECT_FALSE(gen1.dump(&buffer[0], buffer.size() - 1));
    CASE_EXPECT_TRUE(gen1.dump(&buffer[0], buffer.size()));

    for (int i = 0; i < 256; ++i) {
      first_random[i] = static_cast<uint64_t>(gen1.random());
      if (0 != first_random[i]) {
        check_result = true;
      }
    }

    CASE_EXPECT_FALSE(gen1.load(nullptr, buffer.size()));
    CASE_EXPECT_FALSE(gen1.load(&buffer[0], buffer.size() - 1));
    CASE_EXPECT_TRUE(gen1.load(&buffer[0], buffer.size()));
    CASE_EXPECT_TRUE(check_result);

    for (int i = 0; i < 256; ++i) {
      uint64_t second_random = static_cast<uint64_t>(gen1.random());
      CASE_EXPECT_EQ(first_random[i], second_random);
      if (first_random[i] != second_random) {
        break;
      }
    }
  }
}

CASE_TEST(random_test, xoroshiro128_plus) {
  util::random::xoroshiro128_plus gen1;
  gen1.init_seed(0x12345678);

  uint32_t checked1[] = {0xf1c950fb, 0x3c9b4647, 0x212ab3fb, 0x9930ad20, 0xe891a595,
                         0x5f3d9875, 0x264dd138, 0xb23b09e7, 0xb351ccd6, 0xf6342d85};

  for (size_t i = 0; i < sizeof(checked1) / sizeof(checked1[0]); ++i) {
    CASE_EXPECT_EQ(gen1.random(), checked1[i]);
  }

  gen1.get_core().jump();
  uint32_t checked2[] = {0x65981904, 0x2d406a8b, 0xf722cc27, 0xd93f4e0,  0xc8c324e,
                         0x47adc6fc, 0x2e331b89, 0x2de59177, 0x68298f16, 0x3c1f786};

  for (size_t i = 0; i < sizeof(checked2) / sizeof(checked2[0]); ++i) {
    CASE_EXPECT_EQ(gen1.random(), checked2[i]);
  }

  uint32_t arr_seeds[] = {0x01, 0x02, 0x03, 0x04};
  uint32_t *ptr_seeds = &arr_seeds[0];
  gen1.init_seed(ptr_seeds, ptr_seeds + 4);

  uint32_t checked3[] = {0x6ba5dc24, 0x1f1d11fa, 0xfa4932ff, 0x86a802af, 0x784c234,
                         0xbabe3a02, 0x77ad7d11, 0x7ebe938,  0x44211069, 0x2a5ba899};

  for (size_t i = 0; i < sizeof(checked3) / sizeof(checked3[0]); ++i) {
    CASE_EXPECT_EQ(gen1.random(), checked3[i]);
  }

  gen1.get_core().jump();
  uint32_t checked4[] = {0x4956e25b, 0x1637a896, 0xc303f2ef, 0x9f3f6ac7, 0x8048281d,
                         0xfd181cdb, 0xd50c6ccb, 0xa1e5dc16, 0xe2e6feed, 0xe8901128};

  for (size_t i = 0; i < sizeof(checked4) / sizeof(checked4[0]); ++i) {
    CASE_EXPECT_EQ(gen1.random(), checked4[i]);
  }

  std::vector<uint32_t> shuffle_arr;
  for (int i = 0; i < 10; ++i) {
    shuffle_arr.push_back(gen1.random());
  }
  gen1.shuffle(shuffle_arr.begin(), shuffle_arr.end());
  print_vec(CASE_MSG_INFO() << "random_shuffle => ", shuffle_arr);

  // Test load/dump
  {
    std::vector<unsigned char> buffer;
    uint64_t first_random[256] = {0};
    bool check_result = false;
    CASE_EXPECT_GT(gen1.block_size(), 0);
    buffer.resize(gen1.block_size());
    CASE_EXPECT_FALSE(gen1.dump(nullptr, buffer.size()));
    CASE_EXPECT_FALSE(gen1.dump(&buffer[0], buffer.size() - 1));
    CASE_EXPECT_TRUE(gen1.dump(&buffer[0], buffer.size()));

    for (int i = 0; i < 256; ++i) {
      first_random[i] = static_cast<uint64_t>(gen1.random());
      if (0 != first_random[i]) {
        check_result = true;
      }
    }

    CASE_EXPECT_FALSE(gen1.load(nullptr, buffer.size()));
    CASE_EXPECT_FALSE(gen1.load(&buffer[0], buffer.size() - 1));
    CASE_EXPECT_TRUE(gen1.load(&buffer[0], buffer.size()));
    CASE_EXPECT_TRUE(check_result);

    for (int i = 0; i < 256; ++i) {
      uint64_t second_random = static_cast<uint64_t>(gen1.random());
      CASE_EXPECT_EQ(first_random[i], second_random);
      if (first_random[i] != second_random) {
        break;
      }
    }
  }
}

CASE_TEST(random_test, xoshiro256_starstar) {
  util::random::xoshiro256_starstar gen1;
  gen1.init_seed(0x12345678);

  uint64_t checked1[] = {0x83ea18ca6bf49f0c, 0xca24436623484287, 0xe7965c078396b9dc, 0xb9036864adb81725,
                         0x5c35ba435d92ab9b, 0x45202e3b4085a2cc, 0xa1e9410f7f528e4e, 0x48eb35365566f0b2,
                         0x90412e6e9ae131c3, 0xba4a1df61ca0b237};

  for (size_t i = 0; i < sizeof(checked1) / sizeof(checked1[0]); ++i) {
    CASE_EXPECT_EQ(gen1.random(), checked1[i]);
  }

  gen1.get_core().jump();
  uint64_t checked2[] = {0x555e786541686a07, 0x6b5e064426c7dce4, 0x63a8537c3e2de537, 0x1b1eb7c6ef11c6c,
                         0x9d6eaac62ce6a4f4, 0x8b2cb439dc19fadd, 0xb749c288597ceac4, 0x92da83e146fdff1d,
                         0x110d38c051cededc, 0xadab1aacca67bd98};

  for (size_t i = 0; i < sizeof(checked2) / sizeof(checked2[0]); ++i) {
    CASE_EXPECT_EQ(gen1.random(), checked2[i]);
  }

  gen1.get_core().long_jump();
  uint64_t checked3[] = {0x855835566240e421, 0xef37ac417209f23c, 0x27d0b120fbc1263b, 0x3acf06c9c8bebcfc,
                         0x8e4c39858e961d77, 0x54ef99ad7133b336, 0x507e39520e592971, 0xf17538afb5568bf2,
                         0x9b12bf7bc0ad1cb,  0xe30b952ddb0edc22};

  for (size_t i = 0; i < sizeof(checked3) / sizeof(checked3[0]); ++i) {
    CASE_EXPECT_EQ(gen1.random(), checked3[i]);
  }

  uint64_t arr_seeds[] = {0x01, 0x02, 0x03, 0x04};
  uint64_t *ptr_seeds = &arr_seeds[0];
  gen1.init_seed(ptr_seeds, ptr_seeds + 4);

  uint64_t checked4[] = {0x167894d082017430, 0x38b95133d3ac9e80, 0x1f5b58670bd33b,   0xa72aa9c1f3c234dc,
                         0xa369742bfe107109, 0xa10057c6cbaaf5b7, 0xed9d205bdc0671c4, 0x36379ae97a7e84fc,
                         0x68a4d0807b200c28, 0x1178895e2d328ef3};

  for (size_t i = 0; i < sizeof(checked4) / sizeof(checked4[0]); ++i) {
    CASE_EXPECT_EQ(gen1.random(), checked4[i]);
  }

  gen1.get_core().jump();
  uint64_t checked5[] = {0x6892346243ec2224, 0x721f3bb7498cd45b, 0x4706ddfc3ac5a535, 0x1833b360cae1f78f,
                         0x49c783132a986f0f, 0x228581842db7c171, 0x575b41c3f6bafa53, 0xd54e667c5bdc8331,
                         0xd16f385b2194a5ee, 0xd35320c4a01add19};

  for (size_t i = 0; i < sizeof(checked5) / sizeof(checked5[0]); ++i) {
    CASE_EXPECT_EQ(gen1.random(), checked5[i]);
  }

  gen1.get_core().long_jump();
  uint64_t checked6[] = {0xfc07bbe2a0ff49e3, 0x25b74d3c3e1395a4, 0x66c3b4e434a41253, 0xeef93c334db407df,
                         0xcbe33255433c267a, 0x1aeb5a580f8b97f7, 0xee0b16ebb05cc830, 0x1951fff956477d9e,
                         0xd586fc5de6068234, 0xb77c43707de92854};

  for (size_t i = 0; i < sizeof(checked6) / sizeof(checked6[0]); ++i) {
    CASE_EXPECT_EQ(gen1.random(), checked6[i]);
  }

  std::vector<uint64_t> shuffle_arr;
  for (int i = 0; i < 10; ++i) {
    shuffle_arr.push_back(gen1.random());
  }
  gen1.shuffle(shuffle_arr.begin(), shuffle_arr.end());
  print_vec(CASE_MSG_INFO() << "random_shuffle => ", shuffle_arr);

  // Test load/dump
  {
    std::vector<unsigned char> buffer;
    uint64_t first_random[256] = {0};
    bool check_result = false;
    CASE_EXPECT_GT(gen1.block_size(), 0);
    buffer.resize(gen1.block_size());
    CASE_EXPECT_FALSE(gen1.dump(nullptr, buffer.size()));
    CASE_EXPECT_FALSE(gen1.dump(&buffer[0], buffer.size() - 1));
    CASE_EXPECT_TRUE(gen1.dump(&buffer[0], buffer.size()));

    for (int i = 0; i < 256; ++i) {
      first_random[i] = static_cast<uint64_t>(gen1.random());
      if (0 != first_random[i]) {
        check_result = true;
      }
    }

    CASE_EXPECT_FALSE(gen1.load(nullptr, buffer.size()));
    CASE_EXPECT_FALSE(gen1.load(&buffer[0], buffer.size() - 1));
    CASE_EXPECT_TRUE(gen1.load(&buffer[0], buffer.size()));
    CASE_EXPECT_TRUE(check_result);

    for (int i = 0; i < 256; ++i) {
      uint64_t second_random = static_cast<uint64_t>(gen1.random());
      CASE_EXPECT_EQ(first_random[i], second_random);
      if (first_random[i] != second_random) {
        break;
      }
    }
  }
}

CASE_TEST(random_test, xoshiro256_plus) {
  util::random::xoshiro256_plus gen1;
  gen1.init_seed(0x12345678);

  uint64_t checked1[] = {0xa6766515ded04cf7, 0x8b8ef459334146d2, 0xa5e132765ffaea1b, 0x42a32c7899960f6,
                         0xb55425c32b032ae,  0x4f5b510e66ed35c3, 0x86364d228b55294e, 0x15fee24568b9f2e4,
                         0x79172607cb7639c8, 0x380558a4c62a5df3};

  for (size_t i = 0; i < sizeof(checked1) / sizeof(checked1[0]); ++i) {
    CASE_EXPECT_EQ(gen1.random(), checked1[i]);
  }

  gen1.get_core().jump();
  uint64_t checked2[] = {0x65e286c2682c7ede, 0x6fd589cda019d6da, 0x74ca58c461d9116,  0xaf78ff6a0b528fa,
                         0x532d4519bc355dff, 0x4fd9664f92848fd0, 0xe871531860e04f51, 0x9f66375cd3511195,
                         0xedb2c3bd1fad482d, 0xabd051255c4a95a4};

  for (size_t i = 0; i < sizeof(checked2) / sizeof(checked2[0]); ++i) {
    CASE_EXPECT_EQ(gen1.random(), checked2[i]);
  }

  gen1.get_core().long_jump();
  uint64_t checked3[] = {0x823dc8ffe9ec3044, 0x2282021c3619f998, 0x58b7e7b6e4bf12c2, 0xba93e7ae8ffd60be,
                         0x7c5604182a5e5c,   0xfe2b1a4e6cc03509, 0x3cc13d59418b58b6, 0xee043f7276571ff3,
                         0xb0cefcacf43f423d, 0x2c06bd7d4b2a7b61};

  for (size_t i = 0; i < sizeof(checked3) / sizeof(checked3[0]); ++i) {
    CASE_EXPECT_EQ(gen1.random(), checked3[i]);
  }

  uint64_t arr_seeds[] = {0x01, 0x02, 0x03, 0x04};
  uint64_t *ptr_seeds = &arr_seeds[0];
  gen1.init_seed(ptr_seeds, ptr_seeds + 4);

  uint64_t checked4[] = {0xf7d9f594a5851eca, 0x1554ccd2906e61d5, 0xf1a8da801e3b1052, 0xbf31cf72a94cc91b,
                         0x1a8b65873ac842c9, 0xd15befe036b53136, 0x8f8ff0898e5cca09, 0xf287e57ae52130ec,
                         0x4b93d1235a7a9687, 0x4a1e76c36a3f44f9};

  for (size_t i = 0; i < sizeof(checked4) / sizeof(checked4[0]); ++i) {
    CASE_EXPECT_EQ(gen1.random(), checked4[i]);
  }

  gen1.get_core().jump();
  uint64_t checked5[] = {0x5ab99b574ab79179, 0x9dd72e9427eecf2d, 0x847348252d253c3a, 0x45e4172e49119e02,
                         0xfef4edb4819f165e, 0xecd2f9db3f6911a3, 0xaa21cb67c76ca6cf, 0x26a7e2345c58b741,
                         0xaf507d5180f1070c, 0x5c635df444a4f1};

  for (size_t i = 0; i < sizeof(checked5) / sizeof(checked5[0]); ++i) {
    CASE_EXPECT_EQ(gen1.random(), checked5[i]);
  }

  gen1.get_core().long_jump();
  uint64_t checked6[] = {0x7f463ee9f76cdf70, 0xc0a587ad5ee6ec85, 0xb42e076175295e3a, 0x96d379025346fa5b,
                         0xa4de52d9b1d5e2e5, 0xa8790f06ce0d31de, 0x5d0eb9a768b4700f, 0xa4fbadd2392670bd,
                         0xc7b31a2f3b7819ee, 0xcca74cb594188bcd};

  for (size_t i = 0; i < sizeof(checked6) / sizeof(checked6[0]); ++i) {
    CASE_EXPECT_EQ(gen1.random(), checked6[i]);
  }

  std::vector<uint64_t> shuffle_arr;
  for (int i = 0; i < 10; ++i) {
    shuffle_arr.push_back(gen1.random());
  }
  gen1.shuffle(shuffle_arr.begin(), shuffle_arr.end());
  print_vec(CASE_MSG_INFO() << "random_shuffle => ", shuffle_arr);

  // Test load/dump
  {
    std::vector<unsigned char> buffer;
    uint64_t first_random[256] = {0};
    bool check_result = false;
    CASE_EXPECT_GT(gen1.block_size(), 0);
    buffer.resize(gen1.block_size());
    CASE_EXPECT_FALSE(gen1.dump(nullptr, buffer.size()));
    CASE_EXPECT_FALSE(gen1.dump(&buffer[0], buffer.size() - 1));
    CASE_EXPECT_TRUE(gen1.dump(&buffer[0], buffer.size()));

    for (int i = 0; i < 256; ++i) {
      first_random[i] = static_cast<uint64_t>(gen1.random());
      if (0 != first_random[i]) {
        check_result = true;
      }
    }

    CASE_EXPECT_FALSE(gen1.load(nullptr, buffer.size()));
    CASE_EXPECT_FALSE(gen1.load(&buffer[0], buffer.size() - 1));
    CASE_EXPECT_TRUE(gen1.load(&buffer[0], buffer.size()));
    CASE_EXPECT_TRUE(check_result);

    for (int i = 0; i < 256; ++i) {
      uint64_t second_random = static_cast<uint64_t>(gen1.random());
      CASE_EXPECT_EQ(first_random[i], second_random);
      if (first_random[i] != second_random) {
        break;
      }
    }
  }
}

CASE_TEST(random_test, uuid_generator) {
  CASE_MSG_INFO() << "generate_string(true): " << util::random::uuid_generator::generate_string(true) << std::endl;
  CASE_MSG_INFO() << "generate_string(false): " << util::random::uuid_generator::generate_string(false) << std::endl;
  CASE_MSG_INFO() << "generate_string_random(true): " << util::random::uuid_generator::generate_string_random(true)
                  << std::endl;
  CASE_MSG_INFO() << "generate_string_random(false): " << util::random::uuid_generator::generate_string_random(false)
                  << std::endl;

  LIBATFRAME_UTILS_AUTO_SELETC_SET(std::string) uuids;
  for (int i = 0; i < 1000000; ++i) {
    std::string val = util::random::uuid_generator::generate_string_random();
    CASE_EXPECT_TRUE(uuids.end() == uuids.find(val));
    uuids.insert(val);
  }
  CASE_MSG_INFO() << "allocate " << uuids.size() << " finished" << std::endl;
}

CASE_TEST(random_test, uuid_generator_time) {
  CASE_MSG_INFO() << "generate_string_time(true): " << util::random::uuid_generator::generate_string_time(true)
                  << std::endl;
  CASE_MSG_INFO() << "generate_string_time(false): " << util::random::uuid_generator::generate_string_time(false)
                  << std::endl;

  LIBATFRAME_UTILS_AUTO_SELETC_SET(std::string) uuids;
  for (int i = 0; i < 1000000; ++i) {
    std::string val = util::random::uuid_generator::generate_string_time();
    CASE_EXPECT_TRUE(uuids.end() == uuids.find(val));
    uuids.insert(val);
  }
  CASE_MSG_INFO() << "allocate " << uuids.size() << " finished" << std::endl;
}

CASE_TEST(random_test, uuid_generator_random_bechmark) {
  LIBATFRAME_UTILS_AUTO_SELETC_SET(std::string) uuids;
  for (int i = 0; i < 1000000; ++i) {
    std::string val = util::random::uuid_generator::generate_string_random();
    CASE_EXPECT_TRUE(uuids.end() == uuids.find(val));
    uuids.insert(val);
  }
  CASE_MSG_INFO() << "allocate " << 1000000 << " finished" << std::endl;
}

CASE_TEST(random_test, uuid_generator_time_bechmark) {
  for (int i = 0; i < 1000000; ++i) {
    util::random::uuid_generator::generate_string_time();
  }
  CASE_MSG_INFO() << "allocate " << 1000000 << " finished" << std::endl;
}
