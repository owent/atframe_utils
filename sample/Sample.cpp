
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include "log/LogWrapper.h"

#include "Logic/AttributeManager.h"
#include "Random/RandomGenerator.h"
#include "String/TQueryString.h"
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
void RandomSample() {
    printf("\n");
    printf("===============begin random sample==============\n");
    util::random::MT19937 stGen1;
    stGen1.InitSeed(123);

    printf("Random - mt19937: %u\n", stGen1.Random());
    printf("Random - mt19937: %u\n", stGen1());
    printf("Random - mt19937 - between [100, 10000): %d\n", stGen1.RandomBetween(100, 10000));

    printf("===============end random sample==============\n");
}

//=======================================================================================================


//=======================================================================================================
void TQueryStringSample() {
    printf("\n");
    printf("===============begin querystring sample==============\n");

    util::TQueryString encode, decode;

    encode.Set("a", "wulala");
    encode.Set("page", "ok!");

    std::string output;
    encode.Encode(output);

    util::types::ItemArray::ptr_type arr = encode.CreateArray();

    arr->Append("blablabla...");
    arr->Append("a and b is ab");
    util::types::ItemObject::ptr_type obj = encode.CreateObject();
    obj->Set("so", "");
    arr->Append(obj);

    encode.Set("c", arr);

    std::cout << "Encode => " << encode.ToString() << std::endl;
    std::cout << "Encode (old) => " << output << std::endl;
    std::cout << "Array => " << arr->ToString() << std::endl;

    decode.Decode(encode.ToString().c_str());
    std::cout << "Decode => " << decode.ToString() << std::endl;

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
    TQueryStringSample();
    hash_sample();
    LogSample();
    return 0;
}
