#ifndef _RESULT_H
#define _RESULT_H

typedef enum { OK, ERR } ResultVariant;

/// INFO:
/// T ok_value Can be nullptr.
/// char *err_msg Must be a static string or nullptr.
///
/// How to use them properly:
/// When 'variant' is ERR:
/// | define 'err_msg' to a string,
/// | define 'ok_value' to an empty value to satisfy the compiler.
/// When 'variant' is OK:
/// | define 'err_msg' to "",
/// | define 'ok_value' to the intended result.
#define RESULT(T, Name)                                                        \
    typedef struct {                                                           \
        ResultVariant variant;                                                 \
        const char *err_msg;                                                   \
        T ok_value;                                                            \
    } Result##Name;

RESULT(void *, Void)
RESULT(int, Int)
RESULT(double, Double)
RESULT(bool, Bool)

/// ResultHeap... do not need free() on ERR variant
RESULT(char *, HeapString)
RESULT(void *, HeapStruct)

RESULT(int, Enum)

extern const char *UNKNOWN_ERR;

#define RESULT_VOID_DEFAULT                                                    \
    {.variant = ERR, .err_msg = UNKNOWN_ERR, .ok_value = nullptr}

#endif
