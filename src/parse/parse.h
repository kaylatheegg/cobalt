#pragma once
#define PARSE_H

int parse_file(cobalt_ctx* ctx);

#define TOKENS \
    TOKEN(TOK_INVALID, "[INVALID]") \
    /* token: */ \
    TOKEN(TOK_KEYWORD, "keyword") \
    TOKEN(TOK_IDENTIFIER, "identifier") \
    TOKEN(TOK_CONSTANT, "constant") \
    TOKEN(TOK_STR_LIT, "string-literal") \
    TOKEN(TOK_PUNCT, "punctuator") \
    /* preprocessing-token: */ \
    TOKEN(PPTOK_HEADER_NAME, "header-name") \
    TOKEN(PPTOK_IDENTIFIER, "pp-identifier") \
    TOKEN(PPTOK_NUMBER, "pp-number") \
    TOKEN(PPTOK_CHAR_CONST, "character-constant") \
    TOKEN(PPTOK_STR_LIT, "pp-string-literal") \
    TOKEN(PPTOK_PUNCT, "pp-punctuator") \
    /* from here on, these are non-c tokens, but these make parsing easier. */ \
    /* these get transformed during step 3 of compilation, before its supposed to. */ \
    /* we still keep track of the c token type (token:), but we use itype for actual parsing */ \
    /* punct */ \
    TOKEN(CTOK_OPEN_SQUBRACE, "[") \
    TOKEN(CTOK_CLOSE_SQUBRACE, "]") \
    TOKEN(CTOK_OPEN_PAREN, "(") \
    TOKEN(CTOK_CLOSE_PAREN, ")") \
    TOKEN(CTOK_OPEN_BRACE, "{") \
    TOKEN(CTOK_CLOSE_BRACE, "}") \
    TOKEN(CTOK_DOT, ".") \
    TOKEN(CTOK_ARROW, "->") \
    TOKEN(CTOK_INC, "++") \
    TOKEN(CTOK_DEC, "--") \
    TOKEN(CTOK_AMPERSAND, "&") \
    TOKEN(CTOK_TIMES, "*") \
    TOKEN(CTOK_PLUS, "+") \
    TOKEN(CTOK_MINUS, "-") \
    TOKEN(CTOK_TILDE, "~") \
    TOKEN(CTOK_EXCLAM, "!") \
    TOKEN(CTOK_FWSLASH, "/") \
    TOKEN(CTOK_PERCENT, "%") \
    TOKEN(CTOK_LSHIFT, "<<") \
    TOKEN(CTOK_RSHIFT, ">>") \
    TOKEN(CTOK_LESS_THAN, "<") \
    TOKEN(CTOK_GREATER_THAN, ">") \
    TOKEN(CTOK_LESS_EQ, "<=") \
    TOKEN(CTOK_GREATER_EQ, ">=") \
    TOKEN(CTOK_EQ_EQ, "==") \
    TOKEN(CTOK_NOT_EQ, "!=") \
    TOKEN(CTOK_CARET, "^") \
    TOKEN(CTOK_OR, "|") \
    TOKEN(CTOK_AND_AND, "&&") \
    TOKEN(CTOK_OR_OR, "||") \
    TOKEN(CTOK_QUESTION, "?") \
    TOKEN(CTOK_COLON, ":") \
    TOKEN(CTOK_COLON_COLON, "::") \
    TOKEN(CTOK_SEMICOLON, ";") \
    TOKEN(CTOK_ELLIPSIS, "...") \
    TOKEN(CTOK_EQ, "=") \
    TOKEN(CTOK_ASSIGN_TIMES, "*=") \
    TOKEN(CTOK_ASSIGN_DIV, "/=") \
    TOKEN(CTOK_ASSIGN_MOD, "%=") \
    TOKEN(CTOK_ASSIGN_ADD, "+=") \
    TOKEN(CTOK_ASSIGN_SUB, "-=") \
    TOKEN(CTOK_ASSIGN_LSHIFT, "<<=") \
    TOKEN(CTOK_ASSIGN_RSHIFT, ">>=") \
    TOKEN(CTOK_ASSIGN_AND, "&=") \
    TOKEN(CTOK_ASSIGN_XOR, "^=") \
    TOKEN(CTOK_ASSIGN_OR, "|=") \
    TOKEN(CTOK_COMMA, ",") \
    TOKEN(CTOK_HASH, "#") \
    TOKEN(CTOK_HASH_HASH, "##") \
    /* constant types */ \

typedef enum: u8 {
#define TOKEN(tok, str) tok,
    TOKENS
#undef TOKEN
} token_type;


typedef struct {
    token_type type;
    token_type itype;
    string tok;
    u32 line;
} token;

Vec_typedef(token);

typedef struct {
    Vec(token) tokens;
    size_t curr_offset;
    size_t curr_line;
    Vec(string) logical_lines;
    cobalt_ctx* ctx;
} parser_ctx;

void parser_phase1(cobalt_ctx* ctx);
int parser_phase2(parser_ctx* ctx, Vec(string) physical_lines);
int parser_phase3(parser_ctx* ctx);

void print_lexing_error(parser_ctx* ctx, char* format, ...);