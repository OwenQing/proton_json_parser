#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "protonjson.h"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do {\
        test_count++;\
        if (equality)\
            test_pass++;\
        else {\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
        }\
    } while(0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")

#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%f")

/* int memcmp(const void *str1, const void *str2, size_t n) 比较两个存储区的字符串
 * str1 == str2, return 0
 * str1 < str2,  return <0
 * str1 > str2,  return >0
 * */
#define EXPECT_EQ_STRING(expect, actual, alength)\
    EXPECT_EQ_BASE(sizeof(expect) - 1 == alength && memcmp(expect, actual, alength) == 0, expect, actual, "%s")

#define EXPECT_TRUE(actual) EXPECT_EQ_BASE(actual != 0, "true", "false", "%s")  
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE(actual == 0, "false", "true", "%s")

/* TDD test driver development*/
/* 测试驱动开法 */

static void
test_parse_null() {
    ProtonValue v;
    v.type = PROTON_FALSE;
    EXPECT_EQ_INT(PROTON_PARSE_OK, protonParse(&v, "null"));
    EXPECT_EQ_INT(PROTON_NULL, protonGetType(&v));
}

static void
test_parse_expect_value() {
    ProtonValue v;

    v.type = PROTON_FALSE;
    EXPECT_EQ_INT(PROTON_PARSE_EXPECT_VALUE, protonParse(&v, ""));
    EXPECT_EQ_INT(PROTON_NULL, protonGetType(&v));
    
    v.type = PROTON_FALSE;
    EXPECT_EQ_INT(PROTON_PARSE_EXPECT_VALUE, protonParse(&v, " "));
    EXPECT_EQ_INT(PROTON_NULL, protonGetType(&v));
}

#define TEST_ERROR(error, json) \
    do {\
        ProtonValue v;\
        v.type = PROTON_FALSE;\
        EXPECT_EQ_INT(error, protonParse(&v, json));\
        EXPECT_EQ_INT(PROTON_NULL, protonGetType(&v));\
    }\
    while(0)

static void
test_parse_invalid_value() {
    TEST_ERROR(PROTON_PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(PROTON_PARSE_INVALID_VALUE, "?");
}

static void
test_parse_root_not_singular() {
    ProtonValue v;
    v.type = PROTON_FALSE;
    EXPECT_EQ_INT(PROTON_PARSE_ROOT_NOT_SINGULAR, protonParse(&v, "null x"));
    EXPECT_EQ_INT(PROTON_NULL, protonGetType(&v));
}

static void
test_parse_true() {
    ProtonValue v;
    v.type = PROTON_TRUE;
    EXPECT_EQ_INT(PROTON_PARSE_OK, protonParse(&v, "true"));
    EXPECT_EQ_INT(PROTON_TRUE, protonGetType(&v));
}


static void
test_parse_false() {
    ProtonValue v;
    v.type = PROTON_FALSE;
    EXPECT_EQ_INT(PROTON_PARSE_OK, protonParse(&v, "false"));
    EXPECT_EQ_INT(PROTON_FALSE, protonGetType(&v));
}

#define TEST_NUMBER(expect, json) \
    do {\
        ProtonValue v;\
        EXPECT_EQ_INT(PROTON_PARSE_OK, protonParse(&v, json));\
        EXPECT_EQ_DOUBLE(expect, protonGetNumber(&v));\
    } while(0)

static void
test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

    /* 边界值测试 */
    /* the smallest number > 1 */
    TEST_NUMBER(1.0000000000000002, "1.0000000000000002");
    /* minimum denormal */
    TEST_NUMBER( 4.9406564584124654e-324, "4.9406564584124654e-324");
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    /* Max subnormal double */
    TEST_NUMBER( 2.2250738585072009e-308, "2.2250738585072009e-308");
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    /* Min normal positive double */
    TEST_NUMBER( 2.2250738585072014e-308, "2.2250738585072014e-308");
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    /* Max double */
    TEST_NUMBER( 1.7976931348623157e+308, "1.7976931348623157e+308");
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

#define TEST_STRING(expect, json)\
    do{\
        ProtonValue v;\
        proton_init(&v);\
        EXPECT_EQ_INT(PROTON_PARSE_OK, protonParse(&v, json));\
        EXPECT_EQ_STRING(expect, protonGetString(&v), protonGetStringLength(&v));\
        protonFree(&v);\
    } while(0)

static void
test_parse_string() {
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
}

static void
test_access_string() {
    ProtonValue v;
    proton_init(&v);
    protonSetString(&v, "", 0);
    EXPECT_EQ_STRING("", protonGetString(&v), protonGetStringLength(&v));

    protonSetString(&v, "Hello", 5);
    EXPECT_EQ_STRING("Hello", protonGetString(&v), protonGetStringLength(&v));
    protonFree(&v);
}

static void
test_access_boolean() {
    ProtonValue v;
    proton_init(&v);
    protonSetString(&v, "a", 1);
    protonSetBoolean(&v, 1);
    EXPECT_TRUE(protonGetBoolean(&v));
    protonSetBoolean(&v, 0);
    EXPECT_FALSE(protonGetBoolean(&v));
    protonFree(&v);
}

static void
test_access_number() {
    ProtonValue v;
    proton_init(&v);
    protonSetString(&v, "a", 1);
    protonSetNumber(&v, 9.2);
    EXPECT_EQ_DOUBLE(9.2, protonGetNumber(&v));
    protonFree(&v);
}

static void
test_access_null() {
    ProtonValue v;
    proton_init(&v);
    protonSetString(&v, "a", 1);
    protonSetNull(&v);
    EXPECT_EQ_INT(PROTON_NULL, protonGetType(&v));
    protonFree(&v);
}

static void
test_parse() {
    test_parse_null();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_string();
}

static void
test_access() {
    test_access_string();
    test_access_boolean();
    test_access_number();
    test_access_null();
}

static void
test() {
    test_parse();
    test_access();
}

int
main() {
    test();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}
