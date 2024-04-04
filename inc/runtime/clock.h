#pragma once
#include "manifest.h"

begin_c

enum {
    nsec_in_usec = 1000, // nano in micro
    nsec_in_msec = nsec_in_usec * 1000, // nano in milli
    nsec_in_sec  = nsec_in_msec * 1000,
    usec_in_msec = 1000, // micro in mill
    msec_in_sec  = 1000, // milli in sec
    usec_in_sec  = usec_in_msec * msec_in_sec // micro in sec
};

typedef struct {
    double   (*seconds)(void);      // since boot
    uint64_t (*nanoseconds)(void);  // since boot overflows in about 584.5 years
    uint64_t (*unix_microseconds)(void); // since January 1, 1970
    uint64_t (*unix_seconds)(void);      // since January 1, 1970
    uint64_t (*microseconds)(void); // NOT monotonic(!) UTC since epoch January 1, 1601
    uint64_t (*localtime)(void);    // local time microseconds since epoch
    void (*utc)(uint64_t microseconds, int32_t* year, int32_t* month,
        int32_t* day, int32_t* hh, int32_t* mm, int32_t* ss, int32_t* ms,
        int32_t* mc);
    void (*local)(uint64_t microseconds, int32_t* year, int32_t* month,
        int32_t* day, int32_t* hh, int32_t* mm, int32_t* ss, int32_t* ms,
        int32_t* mc);
    void (*test)(void);
} clock_if;

extern clock_if clock;

end_c
