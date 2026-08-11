#ifndef HELPERS_H_INCLUDED
#define HELPERS_H_INCLUDED

#include "constants.h"

void serial_msg(const char* command, const char* data, const char* rem);

#ifdef LOAD_TEST
void test_harness();
#endif

#endif
