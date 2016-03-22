
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include "log/LogWrapper.h"

#include "random/random_generator.h"
#include "string/tquerystring.h"
#include "algorithm/hash.h"

//=======================================================================================================
void LogSample() {
    puts("");
    puts("===============begin log sample==============");

    WLOG_INIT(util::log::LogWrapper::categorize_t::DEFAULT, util::log::LogWrapper::level_t::LOG_LW_DEBUG);

    PSTDERROR("try to print error log.\n");

    WLOGNOTICE("notice log %d", 0);

    puts("===============end log sample==============");
}

//=======================================================================================================

//=======================================================================================================
void random_sample() {
    printf("\n");
    printf("===============begin random sample==============\n");
    util::random::mt19937 gen1;
    gen1.init_seed(123);

    printf("Random - mt19937: %u\n", gen1.random());
    printf("Random - mt19937: %u\n", gen1());
    printf("Random - mt19937 - between [100, 10000): %d\n", gen1.random_between(100, 10000));

    printf("===============end random sample==============\n");
}

//=======================================================================================================


//=======================================================================================================
void tquerystring_sample() {
    printf("\n");
    printf("===============begin querystring sample==============\n");

    util::tquerystring encode, decode;

    encode.set("a", "wulala");
    encode.set("page", "ok!");

    std::string output;
    encode.encode(output);

    util::types::item_array::ptr_type arr = encode.create_array();

    arr->append("blablabla...");
    arr->append("a and b is ab");
    util::types::item_object::ptr_type obj = encode.create_object();
    obj->set("so", "");
    arr->append(obj);

    encode.set("c", arr);

    std::cout << "encode => " << encode.to_string() << std::endl;
    std::cout << "encode (old) => " << output << std::endl;
    std::cout << "Array => " << arr->to_string() << std::endl;

    decode.decode(encode.to_string().c_str());
    std::cout << "decode => " << decode.to_string() << std::endl;

    printf("===============end querystring sample================\n");
}

//=======================================================================================================

//=======================================================================================================
void hash_sample() {
    puts("");
    puts("===============begin hash sample==============");
    char str_buff[] = "Hello World!\nI'm OWenT\n";
    printf("Hashed String: \n%s\n", str_buff);

    std::cout << "FNV-1:   " << util::hash::hash_fnv1<uint32_t>(str_buff, strlen(str_buff)) << std::endl;
    std::cout << "FNV-1A:  " << util::hash::hash_fnv1a<uint32_t>(str_buff, strlen(str_buff)) << std::endl;
    std::cout << "SDBM:    " << util::hash::hash_sdbm<uint32_t>(str_buff, strlen(str_buff)) << std::endl;
    std::cout << "RS:      " << util::hash::hash_rs<uint32_t>(str_buff, strlen(str_buff)) << std::endl;
    std::cout << "JS:      " << util::hash::hash_js<uint32_t>(str_buff, strlen(str_buff)) << std::endl;
    std::cout << "PJW:     " << util::hash::hash_pjw<uint32_t>(str_buff, strlen(str_buff)) << std::endl;
    std::cout << "ELF:     " << util::hash::hash_elf<uint32_t>(str_buff, strlen(str_buff)) << std::endl;
    std::cout << "BKDR:    " << util::hash::hash_bkdr<uint32_t>(str_buff, strlen(str_buff)) << std::endl;
    std::cout << "DJB:     " << util::hash::hash_djb<uint32_t>(str_buff, strlen(str_buff)) << std::endl;
    std::cout << "AP:      " << util::hash::hash_ap<uint32_t>(str_buff, strlen(str_buff)) << std::endl;
    puts("===============end hash sample==============");
}
//=======================================================================================================

int main(int argc, char **argv) {
    tquerystring_sample();
    hash_sample();
    random_sample();
    LogSample();
    return 0;
}
