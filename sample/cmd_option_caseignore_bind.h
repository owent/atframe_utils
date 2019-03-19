#ifndef TEST_CASEIGNORE_BIND_H
#define TEST_CASEIGNORE_BIND_H

#pragma once


#include "cli/cmd_option.h"
#include <cstdio>


void bind_ci_cmd_print(util::cli::callback_param) { puts("ci: do nothing! - free func without parameter\n"); }

void bind_ci_cmd_print2(util::cli::callback_param par, double d) {
    printf("ci: Free Fun B2 Params Num: %d, d => %lf\n", static_cast<int>(par.get_params_number()), d);
}

void bind_ci_cmd_print3(util::cli::callback_param par, double d, int i) {
    printf("ci: Free Fun B3 Params Num: %d, i => %d, d => %lf\n", static_cast<int>(par.get_params_number()), i, d);
}

void bind_ci_cmd_init() {

    util::cli::cmd_option_ci::ptr_type stOpt = util::cli::cmd_option_ci::create();

    stOpt->bind_help_cmd("-h, --help")->set_help_msg("Help (CI):");
    stOpt->bind_cmd("p, print", bind_ci_cmd_print);

    stOpt->bind_cmd("pd, printd", bind_ci_cmd_print2, 1.5);
    stOpt->bind_cmd("pdi, printdi", bind_ci_cmd_print3, 3.5, 105)->set_help_msg("Cmd Ignore Case printdi");

    stOpt->start("-H 1 2 3 5 pDi 4 5 p PD");
}

#endif
