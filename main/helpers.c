#include "helpers.h"

#include <stdio.h>

void serial_msg(const char* command, const char* data, const char* rem)
{
    printf("%s", command);

    if (data != NULL)
        printf(" %s", data);

    if (rem != NULL)
        printf(" ;%s\n", rem);
    else
        printf("\n");
}
