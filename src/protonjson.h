#include <stdlib.h>

#ifndef PROTON_H_
#define PROTON_H_
#define proton_init(v)  do { (v)->type = PROTON_NULL; } while(0)

typedef enum {
    PROTON_NULL,
    PROTON_FALSE,
    PROTON_TRUE,
    PROTON_NUMBER,
    PROTON_STRING,
    PROTON_ARRAY,
    PROTON_OBJECT,
} ProtonType;

typedef struct {
    union {
        struct { char* s; size_t len; } s;  /* string */
        double n;                           /* number */
    } u;
    ProtonType type;
} ProtonValue;

enum {
    PROTON_PARSE_OK = 0,
    PROTON_PARSE_EXPECT_VALUE,
    PROTON_PARSE_INVALID_VALUE,
    PROTON_PARSE_ROOT_NOT_SINGULAR,
    PROTON_PARSE_NUMBER_TOO_BIG,
};

int protonParse(ProtonValue* v, const char* json);

/* proton type */
ProtonType protonGetType(const ProtonValue* v);

/* boolean */
int protonGetBoolean(const ProtonValue* v);
void protonSetBoolean(ProtonValue* v, int b);

/* number */
double protonGetNumber(const ProtonValue* v);
void protonSetNumber(ProtonValue* v, double n);

/* string */
const char* protonGetString(const ProtonValue* v);
size_t protonGetStringLength(const ProtonValue* v);
void protonSetString(ProtonValue* v, const char* s, size_t len);

#endif /* PROTON_H_ */
