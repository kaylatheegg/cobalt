#pragma once
#define PARSE_H

int parse_file(cobalt_ctx* ctx);

#define TOKENS \
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
    /* these get transformed during step 7 of compilation, before actual parsing occurs. */ \
    /* we still keep track of the c token type (token:), but we use itype for actual parsing */ 

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
