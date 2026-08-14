#include "helpers.h"

#include <stdio.h>

#include "config.h"
#include "constants.h"

#ifdef SERIAL_NOTES_ENABLED
void h_serial_msg(const char* command, const char* data, const char* rem)
{
    printf("%s", command);

    if (data != NULL)
        printf(" %s", data);

    if (rem != NULL)
        printf(" ;%s", rem);

    printf("\n");
}
#endif
