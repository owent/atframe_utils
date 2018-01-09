
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include "log/log_sink_file_backend.h"
#include "log/log_wrapper.h"
#include "std/thread.h"


#include "common/file_system.h"
#include "time/time_utility.h"

#include "algorithm/hash.h"
#include "random/random_generator.h"
#include "string/tquerystring.h"


//=======================================================================================================
void log_sample_func1(int times) {
	if (times > 0) {
		log_sample_func1(times - 1);
		return;
	}

	puts("");
	puts("===============begin log sample==============");

	WLOG_INIT(util::log::log_wrapper::categorize_t::DEFAULT, util::log::log_wrapper::level_t::LOG_LW_DEBUG);
	WLOG_GETCAT(util::log::log_wrapper::categorize_t::DEFAULT)->set_stacktrace_level(util::log::log_wrapper::level_t::LOG_LW_INFO);

	PSTDERROR("try to print error log.\n");
	PSTDOK("try to print ok log.\n");

	WLOGNOTICE("notice log %d", 0);

	util::log::log_sink_file_backend filed_backend;
	filed_backend.set_max_file_size(256);
	filed_backend.set_rotate_size(3);
	filed_backend.set_file_pattern("%Y-%m-%d/%S/%N.log");

	WLOG_GETCAT(util::log::log_wrapper::categorize_t::DEFAULT)->add_sink(filed_backend);

	for (int i = 0; i < 16; ++i) {
		WLOGDEBUG("first dir test log: %d", i);
	}

	THREAD_SLEEP_MS(1000);
	util::time::time_utility::update();

	for (int i = 0; i < 16; ++i) {
		WLOGDEBUG("second dir log: %d", i);
	}

	unsigned long long ull_test_in_mingw = 64;
	WLOGINFO("%llu", ull_test_in_mingw);
	printf("log are located at %s\n", util::file_system::get_cwd().c_str());
	puts("===============end log sample==============");
}

class log_sample_functor2 {
public:
	void func2(int times) {
		if (times & 0x01) {
			func2(times - 1);
		} else {
			log_sample_func1(times - 1);
		}
	}
};

class log_sample_functor3 {
public:
	static void func3(int times) {
		if (times & 0x01) {
			func3(times - 1);
		} else {
			log_sample_functor2 f;
			f.func2(times - 1);
		}
	}
};

struct log_sample_functor4 {
	void operator()(int times) {
		if (times & 0x01) {
			(*this)(times - 1);
		} else {
			log_sample_functor3::func3(times - 1);
		}
	}
};

static void log_sample_func5(int times) {
	if (times & 0x01) {
		log_sample_func5(times - 1);
	} else {
		log_sample_functor4 f;
		f(times - 1);
	}
}

void log_sample() {
	log_sample_func5(9);
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

//=======================================================================================================
extern int cmd_option_sample_main();

void cmd_option_sample() {
    puts("");
    puts("===============begin cmd_option sample==============");
    cmd_option_sample_main();
    puts("===============end cmd_option sample==============");
}
//=======================================================================================================


int main(int argc, char **argv) {
    util::time::time_utility::update();

    tquerystring_sample();
    hash_sample();
    random_sample();
    log_sample();
    cmd_option_sample();
    return 0;
}
