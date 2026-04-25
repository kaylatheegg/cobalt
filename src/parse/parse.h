#pragma once
#define PARSE_H

#define PUNCT \
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

#define KEYWORDS \
    TOKEN(CTOK_ALIGNAS, "alignas") \
    TOKEN(CTOK_ALIGNOF, "alignof") \
    TOKEN(CTOK_AUTO, "auto") \
    TOKEN(CTOK_BOOL, "bool") \
    TOKEN(CTOK_BREAK, "break") \
    TOKEN(CTOK_CASE, "case") \
    TOKEN(CTOK_CHAR, "char") \
    TOKEN(CTOK_CONST, "const") \
    TOKEN(CTOK_CONSTEXPR, "constexpr") \
    TOKEN(CTOK_CONTINUE, "continue") \
    TOKEN(CTOK_DEFAULT, "default") \
    TOKEN(CTOK_DO, "do") \
    TOKEN(CTOK_DOUBLE, "double") \
    TOKEN(CTOK_ELSE, "else") \
    TOKEN(CTOK_ENUM, "enum") \
    TOKEN(CTOK_EXTERN, "extern") \
    TOKEN(CTOK_FALSE, "false") \
    TOKEN(CTOK_FLOAT, "float") \
    TOKEN(CTOK_FOR, "for") \
    TOKEN(CTOK_GOTO, "goto") \
    TOKEN(CTOK_IF, "if") \
    TOKEN(CTOK_INLINE, "inline") \
    TOKEN(CTOK_INT, "int") \
    TOKEN(CTOK_LONG, "long") \
    TOKEN(CTOK_NULLPTR, "nullptr") \
    TOKEN(CTOK_REGISTER, "register") \
    TOKEN(CTOK_RESTRICT, "restrict") \
    TOKEN(CTOK_RETURN, "return") \
    TOKEN(CTOK_SHORT, "short") \
    TOKEN(CTOK_SIGNED, "signed") \
    TOKEN(CTOK_SIZEOF, "sizeof") \
    TOKEN(CTOK_STATIC, "static") \
    TOKEN(CTOK_STATIC_ASSERT, "static_assert") \
    TOKEN(CTOK_STRUCT, "struct") \
    TOKEN(CTOK_SWITCH, "switch") \
    TOKEN(CTOK_THREAD_LOCAL, "thread_local") \
    TOKEN(CTOK_TRUE, "true") \
    TOKEN(CTOK_TYPEDEF, "typedef") \
    TOKEN(CTOK_TYPEOF, "typeof") \
    TOKEN(CTOK_TYPEOF_UNQUAL, "typeof_unqual") \
    TOKEN(CTOK_UNION, "union") \
    TOKEN(CTOK_UNSIGNED, "unsigned") \
    TOKEN(CTOK_VOID, "void") \
    TOKEN(CTOK_VOLATILE, "volatile") \
    TOKEN(CTOK_WHILE, "while") \
    TOKEN(CTOK_ATOMIC, "_Atomic") \
    TOKEN(CTOK_BITINT, "_BitInt") \
    TOKEN(CTOK_COMPLEX, "_Complex") \
    TOKEN(CTOK_DECIMAL128, "_Decimal128") \
    TOKEN(CTOK_DECIMAL32, "_Decimal32") \
    TOKEN(CTOK_DECIMAL64, "_Decimal64") \
    TOKEN(CTOK_GENERIC, "_Generic") \
    TOKEN(CTOK_IMAGINARY, "_Imaginary") \
    TOKEN(CTOK_NORETURN, "_Noreturn") \

#define TOKENS \
    TOKEN(TOK_INVALID, "[INVALID]") \
    TOKEN(TOK_WHITESPACE, "whitespace") \
    /* token: */ \
    TOKEN(TOK_IDENTIFIER, "identifier") \
    TOKEN(TOK_CONSTANT, "constant") \
    TOKEN(TOK_STR_LIT, "string-literal") \
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
    PUNCT \
    /* constant types */ \
    /* C reserved keywords */ \
    KEYWORDS
     
#define TOKEN_EXPANDER EVAL(TOKENS)
    
typedef enum: u8 {
#define TOKEN(tok, str) tok,
    TOKEN_EXPANDER
#undef TOKEN
} token_type;

typedef struct {
    token_type type;
    token_type itype;
    string tok;
    bool after_newline;
    u32 line;
    bool was_included;
    string included_from;
    bool from_macro_param;
} token;

typedef struct {
    bool is_function;
    bool is_variadic;
    Vec(token) arguments;
    Vec(token) replacement_list;
    token name;
} macro_define;

typedef struct _parser_ctx {
    Vec(token) tokens;
    size_t curr_tok_index;
    size_t curr_offset;
    size_t curr_line;
    Vec(string) logical_lines;
    cobalt_ctx* ctx;
    Vec(string) pragma_files;
    Vec(macro_define) defines;
    token curr_macro_name;
} parser_ctx;

extern char* token_str[];
extern char* token_enum_str[];

int parse_file(cobalt_ctx* ctx, bool is_include);

void parser_phase1(cobalt_ctx* ctx);
int parser_phase2(parser_ctx* ctx, Vec(string) physical_lines);
int parser_phase3(parser_ctx* ctx);
int parser_phase4(parser_ctx* ctx);
int parser_phase5(parser_ctx* ctx);
int parser_phase6(parser_ctx* ctx);
int parser_phase7(parser_ctx* ctx);

void print_token_stream(parser_ctx* ctx);

int handle_include(parser_ctx* ctx, size_t hash_location);
int handle_define(parser_ctx* ctx);

#define pp_stream(tok) print_token_stream(&(parser_ctx){.tokens = (tok)})

void print_lexing_error(parser_ctx* ctx, char* format, ...);
void print_parsing_error(parser_ctx* ctx, token err_tok, char* format, ...);
void print_parsing_warning(parser_ctx* ctx, token err_tok, char* format, ...);