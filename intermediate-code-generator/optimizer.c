#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "optimizer.h"

#define MAX_STATEMENTS 2000
#define MAX_NAME 128
#define MAX_TOKEN_TEXT 128
#define MAX_CONSTANTS 2000
#define MAX_EXPRESSIONS 2000

extern FILE *yyin;
extern char *yytext;
int yylex(void);

typedef struct {
    int type;
    char text[MAX_TOKEN_TEXT];
} Token;

typedef enum {
    ST_ASSIGN,
    ST_PRINT
} StatementType;

typedef struct {
    StatementType type;
    char dest[MAX_NAME];
    char left[MAX_NAME];
    char right[MAX_NAME];
    char op;
    int is_binary;
    int removed;
} Statement;

typedef struct {
    char name[MAX_NAME];
    int value;
    int valid;
} Constant;

typedef struct {
    char left[MAX_NAME];
    char right[MAX_NAME];
    char result[MAX_NAME];
    char op;
    int valid;
} Expression;

static Statement statements[MAX_STATEMENTS];
static int statement_count = 0;
static Token current_token;
static int source_line = 1;
static int parse_failed = 0;

static void copy_text(char *destination, size_t capacity, const char *source)
{
    if (capacity == 0) return;
    if (!source) source = "";
    {
        size_t length = strlen(source);
        if (length >= capacity) length = capacity - 1;
        memcpy(destination, source, length);
        destination[length] = '\0';
    }
}

static void advance_token(void)
{
    int token_type = yylex();
    current_token.type = token_type;
    copy_text(current_token.text, sizeof(current_token.text), yytext);
    if (token_type == T_EOL) source_line++;
}

static void parse_error(const char *message)
{
    fprintf(stderr, "Parse error near line %d: %s\n", source_line, message);
    parse_failed = 1;
}

static int is_identifier_token(int token_type)
{
    return token_type == T_ID;
}

static int parse_operand(char *out, size_t out_size)
{
    if (current_token.type == T_ID || current_token.type == T_NUMBER) {
        copy_text(out, out_size, current_token.text);
        advance_token();
        return 1;
    }

    /* Allow a simple negative integer, such as: x = -12 */
    if (current_token.type == T_MINUS) {
        char number[MAX_TOKEN_TEXT];
        advance_token();
        if (current_token.type != T_NUMBER) return 0;
        copy_text(number, sizeof(number), current_token.text);
        if (strlen(number) + 2 > out_size) return 0;
        out[0] = '-';
        copy_text(out + 1, out_size - 1, number);
        advance_token();
        return 1;
    }

    return 0;
}

static int append_statement(const Statement *statement)
{
    if (statement_count >= MAX_STATEMENTS) {
        fprintf(stderr, "Error: too many statements (maximum %d).\n", MAX_STATEMENTS);
        return 0;
    }
    statements[statement_count++] = *statement;
    return 1;
}

static void skip_to_end_of_line(void)
{
    while (current_token.type != 0 && current_token.type != T_EOL) {
        advance_token();
    }
}

static int parse_program(void)
{
    advance_token();

    while (current_token.type != 0 && !parse_failed) {
        Statement statement;
        memset(&statement, 0, sizeof(statement));

        if (current_token.type == T_EOL) {
            advance_token();
            continue;
        }

        if (current_token.type == T_PRINT) {
            statement.type = ST_PRINT;
            advance_token();

            if (!parse_operand(statement.left, sizeof(statement.left))) {
                parse_error("expected a variable or integer after 'print'.");
                skip_to_end_of_line();
                continue;
            }
        } else if (is_identifier_token(current_token.type)) {
            statement.type = ST_ASSIGN;
            copy_text(statement.dest, sizeof(statement.dest), current_token.text);
            advance_token();

            if (current_token.type != T_ASSIGN) {
                parse_error("expected '=' after the destination variable.");
                skip_to_end_of_line();
                continue;
            }
            advance_token();

            if (!parse_operand(statement.left, sizeof(statement.left))) {
                parse_error("expected a variable or integer on the right-hand side.");
                skip_to_end_of_line();
                continue;
            }

            if (current_token.type == T_PLUS || current_token.type == T_MINUS ||
                current_token.type == T_STAR || current_token.type == T_SLASH ||
                current_token.type == T_PERCENT) {
                statement.is_binary = 1;
                switch (current_token.type) {
                    case T_PLUS: statement.op = '+'; break;
                    case T_MINUS: statement.op = '-'; break;
                    case T_STAR: statement.op = '*'; break;
                    case T_SLASH: statement.op = '/'; break;
                    case T_PERCENT: statement.op = '%'; break;
                    default: break;
                }
                advance_token();
                if (!parse_operand(statement.right, sizeof(statement.right))) {
                    parse_error("expected a variable or integer after the operator.");
                    skip_to_end_of_line();
                    continue;
                }
            }
        } else {
            parse_error("expected an assignment or a 'print' statement.");
            skip_to_end_of_line();
            continue;
        }

        if (current_token.type != T_EOL && current_token.type != 0) {
            parse_error("unexpected text at the end of the statement.");
            skip_to_end_of_line();
            continue;
        }

        if (!append_statement(&statement)) return 0;
        if (current_token.type == T_EOL) advance_token();
    }

    return !parse_failed;
}

static int parse_integer(const char *text, int *value)
{
    char *end = NULL;
    long parsed;

    if (!text || !*text) return 0;
    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0' || parsed < INT_MIN || parsed > INT_MAX) {
        return 0;
    }
    *value = (int)parsed;
    return 1;
}

static int is_integer(const char *text)
{
    int ignored;
    return parse_integer(text, &ignored);
}

static int evaluate(char op, int left, int right, int *result)
{
    long long wide_result;

    switch (op) {
        case '+': wide_result = (long long)left + right; break;
        case '-': wide_result = (long long)left - right; break;
        case '*': wide_result = (long long)left * right; break;
        case '/':
            if (right == 0 || (left == INT_MIN && right == -1)) return 0;
            *result = left / right;
            return 1;
        case '%':
            if (right == 0 || (left == INT_MIN && right == -1)) return 0;
            *result = left % right;
            return 1;
        default: return 0;
    }

    if (wide_result < INT_MIN || wide_result > INT_MAX) return 0;
    *result = (int)wide_result;
    return 1;
}

static int find_constant(Constant constants[], const char *name)
{
    int i;
    for (i = 0; i < MAX_CONSTANTS; i++) {
        if (constants[i].valid && strcmp(constants[i].name, name) == 0) return i;
    }
    return -1;
}

static void remove_constant(Constant constants[], const char *name)
{
    int index = find_constant(constants, name);
    if (index >= 0) constants[index].valid = 0;
}

static void set_constant(Constant constants[], const char *name, int value)
{
    int i;
    int free_slot = -1;

    for (i = 0; i < MAX_CONSTANTS; i++) {
        if (constants[i].valid && strcmp(constants[i].name, name) == 0) {
            constants[i].value = value;
            return;
        }
        if (!constants[i].valid && free_slot < 0) free_slot = i;
    }

    if (free_slot >= 0) {
        constants[free_slot].valid = 1;
        copy_text(constants[free_slot].name, sizeof(constants[free_slot].name), name);
        constants[free_slot].value = value;
    }
}

static void substitute_constant(char *operand, size_t operand_size, Constant constants[])
{
    int index;
    char number[40];

    if (is_integer(operand)) return;
    index = find_constant(constants, operand);
    if (index < 0) return;

    snprintf(number, sizeof(number), "%d", constants[index].value);
    copy_text(operand, operand_size, number);
}

static void normalize_expression(char *left, char *right, char op)
{
    char temporary[MAX_NAME];

    if ((op == '+' || op == '*') && strcmp(left, right) > 0) {
        copy_text(temporary, sizeof(temporary), left);
        copy_text(left, MAX_NAME, right);
        copy_text(right, MAX_NAME, temporary);
    }
}

static int find_expression(Expression expressions[], const char *left, const char *right, char op)
{
    int i;
    for (i = 0; i < MAX_EXPRESSIONS; i++) {
        if (expressions[i].valid && expressions[i].op == op &&
            strcmp(expressions[i].left, left) == 0 &&
            strcmp(expressions[i].right, right) == 0) {
            return i;
        }
    }
    return -1;
}

static void invalidate_expressions(Expression expressions[], const char *variable)
{
    int i;
    for (i = 0; i < MAX_EXPRESSIONS; i++) {
        if (expressions[i].valid &&
            (strcmp(expressions[i].left, variable) == 0 ||
             strcmp(expressions[i].right, variable) == 0 ||
             strcmp(expressions[i].result, variable) == 0)) {
            expressions[i].valid = 0;
        }
    }
}

static void add_expression(Expression expressions[], const char *left, const char *right,
                           char op, const char *result)
{
    int i;
    for (i = 0; i < MAX_EXPRESSIONS; i++) {
        if (!expressions[i].valid) {
            expressions[i].valid = 1;
            expressions[i].op = op;
            copy_text(expressions[i].left, sizeof(expressions[i].left), left);
            copy_text(expressions[i].right, sizeof(expressions[i].right), right);
            copy_text(expressions[i].result, sizeof(expressions[i].result), result);
            return;
        }
    }
}

/* Constant propagation/folding and common sub-expression elimination. */
static void optimize_expressions(void)
{
    Constant constants[MAX_CONSTANTS] = {0};
    Expression expressions[MAX_EXPRESSIONS] = {0};
    int i;

    for (i = 0; i < statement_count; i++) {
        Statement *statement = &statements[i];

        if (statement->type == ST_PRINT) {
            substitute_constant(statement->left, sizeof(statement->left), constants);
            continue;
        }

        if (!statement->is_binary) {
            int value;
            substitute_constant(statement->left, sizeof(statement->left), constants);
            remove_constant(constants, statement->dest);
            invalidate_expressions(expressions, statement->dest);
            if (parse_integer(statement->left, &value)) {
                set_constant(constants, statement->dest, value);
            }
            continue;
        }

        /* Resolve variables whose values are known constants. */
        substitute_constant(statement->left, sizeof(statement->left), constants);
        substitute_constant(statement->right, sizeof(statement->right), constants);

        {
            int left_value, right_value, folded_value;
            if (parse_integer(statement->left, &left_value) &&
                parse_integer(statement->right, &right_value) &&
                evaluate(statement->op, left_value, right_value, &folded_value)) {
                char folded_text[40];
                snprintf(folded_text, sizeof(folded_text), "%d", folded_value);
                statement->is_binary = 0;
                statement->op = '\0';
                statement->right[0] = '\0';
                copy_text(statement->left, sizeof(statement->left), folded_text);
                remove_constant(constants, statement->dest);
                invalidate_expressions(expressions, statement->dest);
                set_constant(constants, statement->dest, folded_value);
                continue;
            }
        }

        normalize_expression(statement->left, statement->right, statement->op);
        {
            int match = find_expression(expressions, statement->left,
                                        statement->right, statement->op);
            if (match >= 0 && strcmp(expressions[match].result, statement->dest) != 0) {
                char previous_result[MAX_NAME];
                copy_text(previous_result, sizeof(previous_result), expressions[match].result);

                statement->is_binary = 0;
                statement->op = '\0';
                statement->right[0] = '\0';
                copy_text(statement->left, sizeof(statement->left), previous_result);

                remove_constant(constants, statement->dest);
                invalidate_expressions(expressions, statement->dest);
            } else {
                remove_constant(constants, statement->dest);
                invalidate_expressions(expressions, statement->dest);
                add_expression(expressions, statement->left, statement->right,
                               statement->op, statement->dest);
            }
        }
    }
}

static int live_contains(char live[][MAX_NAME], int live_count, const char *name)
{
    int i;
    for (i = 0; i < live_count; i++) {
        if (strcmp(live[i], name) == 0) return 1;
    }
    return 0;
}

static void live_add(char live[][MAX_NAME], int *live_count, const char *name)
{
    if (!name[0] || is_integer(name) || live_contains(live, *live_count, name)) return;
    if (*live_count < MAX_STATEMENTS) {
        copy_text(live[*live_count], MAX_NAME, name);
        (*live_count)++;
    }
}

static void live_remove(char live[][MAX_NAME], int *live_count, const char *name)
{
    int i;
    for (i = 0; i < *live_count; i++) {
        if (strcmp(live[i], name) == 0) {
            int j;
            for (j = i; j < *live_count - 1; j++) {
                copy_text(live[j], MAX_NAME, live[j + 1]);
            }
            (*live_count)--;
            return;
        }
    }
}

/* Backward liveness analysis removes assignments whose values are never used. */
static void eliminate_dead_code(void)
{
    char live[MAX_STATEMENTS][MAX_NAME];
    int live_count = 0;
    int i;

    for (i = statement_count - 1; i >= 0; i--) {
        Statement *statement = &statements[i];

        if (statement->type == ST_PRINT) {
            live_add(live, &live_count, statement->left);
            continue;
        }

        if (!live_contains(live, live_count, statement->dest)) {
            statement->removed = 1;
            continue;
        }

        live_remove(live, &live_count, statement->dest);
        live_add(live, &live_count, statement->left);
        if (statement->is_binary) {
            live_add(live, &live_count, statement->right);
        }
    }
}

static void print_optimized_program(void)
{
    int i;
    for (i = 0; i < statement_count; i++) {
        const Statement *statement = &statements[i];
        if (statement->removed) continue;

        if (statement->type == ST_PRINT) {
            printf("print %s\n", statement->left);
        } else if (statement->is_binary) {
            printf("%s = %s %c %s\n", statement->dest, statement->left,
                   statement->op, statement->right);
        } else {
            printf("%s = %s\n", statement->dest, statement->left);
        }
    }
}

int main(int argc, char *argv[])
{
    int i;
    int removed_count = 0;

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [input-file.tac]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (argc == 2) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            perror("Could not open input file");
            return EXIT_FAILURE;
        }
    }

    if (!parse_program()) {
        if (argc == 2) fclose(yyin);
        return EXIT_FAILURE;
    }

    optimize_expressions();
    eliminate_dead_code();

    for (i = 0; i < statement_count; i++) {
        if (statements[i].removed) removed_count++;
    }

    print_optimized_program();
    fprintf(stderr, "\nOptimization complete: %d input statements, %d removed by dead-code elimination.\n",
            statement_count, removed_count);

    if (argc == 2) fclose(yyin);
    return EXIT_SUCCESS;
}
