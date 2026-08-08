#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

#ifdef DEBUG
#define ERR_CHECK(x) ESP_ERROR_CHECK(x)
#else
#define ERR_CHECK(x)
#endif

#endif
