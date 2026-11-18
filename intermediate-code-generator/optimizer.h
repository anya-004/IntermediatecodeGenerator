#ifndef OPTIMIZER_H
#define OPTIMIZER_H

/* Token values returned by the Flex scanner. */
enum TokenType {
    T_EOL = 256,
    T_ID,
    T_NUMBER,
    T_PRINT,
    T_ASSIGN,
    T_PLUS,
    T_MINUS,
    T_STAR,
    T_SLASH,
    T_PERCENT,
    T_UNKNOWN
};

#endif
