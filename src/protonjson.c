#include "protonjson.h"
#include <stdlib.h> /* NULL, strod() */
#include <math.h>   /* HUGE_VAL */
#include <errno.h>  /* errno, ERANGE */
#include <assert.h> /* assert */
#include <string.h> /* memcpy() */

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')

#ifndef PROTON_PARSE_STACK_INIT_SIZE
#define PROTON_PARSE_STACK_INIT_SIZE 256
#endif

/* 给指针位置的地方复制 */
#define PUTC(c, ch) do { *(char*)protonContextPush(c, sizeof(char)) = (ch); } while(0)

typedef struct {
    const char* json;
    /* 解析 string 不知道长度多大，先把数据刷到
     * stack 中，最后将数据刷到 ProtonValue 中
     * 注: 栈的初始化与内存释放
     * */
    char* stack;
    size_t size, top;
} ProtonContext;

/* stack 空间的分配 */
static void*
protonContextPush(ProtonContext* c, size_t size) {
    void* ret;
    assert(size > 0);
    if (c->top + size > c->size) {
        if (c->size == 0) 
            c->size = PROTON_PARSE_STACK_INIT_SIZE;
        while (c->top + size >= c->size) 
            c->size += c->size >> 1;    /* c->size * 1.5 */
        c->stack = (char*)realloc(c->stack, c->size);
    }
    ret = c->stack + c->top;
    c->top += size;
    return ret;
}

static void*
protonContextPop(ProtonContext* c, size_t size) {
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}

/* 解析字符串 */
static int
parseString(ProtonContext* c, ProtonValue* v) {
    size_t head = c->top, len;
    const char* p;
    EXPECT(c, '\"'); /* 检测字符串 */
    p = c->json;
    for (;;) {
        char ch = *p++;
        switch(ch) {
            case '\"':
                // 解析完成栈清空
                // 这里的 c->top 和初始的 c->top 已经不同了
                len = c->top - head;
                protonSetString(v, (const char*)protonContextPop(c, len), len);
                c->json = p;
                return PROTON_PARSE_OK;
            // 转义 "\\t" --> "\t"
            case '\\':
                switch(*p++) {
                    case '\"': PUTC(c, '\"'); break;
                    case '\\': PUTC(c, '\\'); break;
                    case '/': PUTC(c, '/'); break;
                    case 'b': PUTC(c, '\b'); break;
                    case 'f': PUTC(c, '\f'); break;
                    case 'n': PUTC(c, '\n'); break;
                    case 'r': PUTC(c, '\r'); break;
                    case 't': PUTC(c, '\t'); break;
                    default:
                        c->top = head;
                        return PROTON_PARSE_INVALID_STRING_ESCAPE;
                } 
                break;
            case '\0':
                c->top = head;
                return PROTON_PARSE_MISS_QUOTATION_MARK;
            default:
                // 非法字符处理
                // char 带不带符号是编译器决定的
                if ((unsigned char)ch < 0x20) { 
                    c->top = head;
                    return PROTON_PARSE_INVALID_STRING_CHAR;
                }
                PUTC(c, ch); /* push stack */
        }
    }
}

/* 解析空白符 */
static void
parseWhiteSpace(ProtonContext* c) {
    const char* p = c->json;
    // ignore those char
    while(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

/* 解析字符 */
static int
parseLiteral(ProtonContext* c, ProtonValue* v, const char* literal, ProtonType type) {
    EXPECT(c, literal[0]);
    size_t i;
    for (i = 0; literal[i+1]; i++) {
        // literal i+1 因为 EXPECT 向前移动了一步
        if (c->json[i] != literal[i+1])
            return PROTON_PARSE_INVALID_VALUE;
    }
    c->json += i;
    v->type = type;
    return PROTON_PARSE_OK;
}

/* 解析数字 */
static int
parseNumber(ProtonContext* c, ProtonValue* v) {
    char* end;
    /* valid number */
    // string --> double
    v->u.n = strtod(c->json, &end);
    if (c->json == end) {
        // string have no number
        return PROTON_PARSE_INVALID_VALUE;
    }
    // ?
    c->json = end;
    v->type = PROTON_NUMBER;
    return PROTON_PARSE_OK;
}

/* 解析数字2 */
static int
parseNumber2(ProtonContext* c, ProtonValue* v) {
    const char* p = c->json;
    /* 负号... */
    /* 整数... */
    /* 小数... */
    /* 指数... */
    if (*p == '-') p++;
    if (*p == '0') p++;
    else {
        // 去0后数字不是 1-9 开头，直接报错
        if (!ISDIGIT1TO9(*p)) return PROTON_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    // 小数点检测
    if (*p == '.') {
        p++;
        // 下一位不是数字直接报错
        if (!ISDIGIT(*p)) return PROTON_PARSE_INVALID_VALUE;
        for(p++; ISDIGIT(*p); p++);
    }
    // 科学计数法
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        if (!ISDIGIT1TO9(*p)) return PROTON_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    errno = 0;
    v->u.n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))
        return PROTON_PARSE_NUMBER_TOO_BIG;
    v->type = PROTON_NUMBER;
    c->json = p;
    return PROTON_PARSE_OK;
}

static int
parseValue(ProtonContext* c, ProtonValue* v) {
    switch (*c->json) {
        case 'n':   return parseLiteral(c, v, "null", PROTON_NULL);
        case 't':   return parseLiteral(c, v, "true", PROTON_TRUE);
        case 'f':   return parseLiteral(c, v, "false", PROTON_FALSE);
        case '\0':  return PROTON_PARSE_EXPECT_VALUE;
        case '\"':  return parseString(c, v);
        default:    return parseNumber2(c, v);
    }
}

int
protonParse(ProtonValue* v, const char* json) {
    ProtonContext c;
    assert(v != NULL);
    c.json = json;
    c.stack = NULL;
    c.size = c.top = 0; /* <- */
    // parse failed set null
    v->type = PROTON_NULL;
    parseWhiteSpace(&c);
    int ret;
    if ((ret = parseValue(&c, v)) == PROTON_PARSE_OK) {
        parseWhiteSpace(&c);
        // 检测小尾巴 null x 
        if (*c.json != '\0') {
            return PROTON_PARSE_ROOT_NOT_SINGULAR;
        }
    }

    assert(c.top == 0); /* 保证所有数据都弹出 */
    free(c.stack);
    return ret;
}

ProtonType
protonGetType(const ProtonValue* v) {
    assert(v != NULL);
    return v->type;
}

void
protonFree(ProtonValue* v) {
    assert(v != NULL);
    if (v->type == PROTON_STRING)
        free(v->u.s.s);
    v->type = PROTON_NULL;
}

/* copy clang string */
void
protonSetString(ProtonValue* v, const char* s, size_t len) {
    assert(v != NULL && (s != NULL || len == 0));
    protonFree(v);
    v->u.s.s = (char*)malloc(len + 1);
    memcpy(v->u.s.s, s, len);
    v->u.s.s[len] = '\0';
    v->u.s.len = len;
    v->type = PROTON_STRING;
}

const char*
protonGetString(const ProtonValue* v) {
    assert(v != NULL && v->type == PROTON_STRING);
    return v->u.s.s;
}

size_t
protonGetStringLength(const ProtonValue* v) {
    assert(v != NULL && v->type == PROTON_STRING);
    return v->u.s.len;
}

/* boolean */
void
protonSetBoolean(ProtonValue* v, int b) {
    protonFree(v);
    v->type = b ? PROTON_TRUE : PROTON_FALSE;
}

int
protonGetBoolean(const ProtonValue* v) {
    assert(v != NULL && (v->type == PROTON_TRUE || v->type == PROTON_FALSE));
    return v->type == PROTON_TRUE;
}

/* number */
void
protonSetNumber(ProtonValue* v, double n) {
    protonFree(v);
    v->u.n = n;
    v->type = PROTON_NUMBER;
}

double
protonGetNumber(const ProtonValue* v) {
    assert(v != NULL && v->type == PROTON_NUMBER);
    return v->u.n;
}

void
protonSetNull(ProtonValue* v) {
    protonFree(v);
}

