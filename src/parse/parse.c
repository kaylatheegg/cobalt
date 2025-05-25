#include <ctype.h>

#include "orbit.h"
#include "cobalt.h"
#include "parse.h"

char* token_str[] = {
#define TOKEN(tok, str) str,
    TOKENS
#undef TOKEN
};

// we will NOT enforce translation limits in this compiler, as per C23 5.2.5.2 footnote 13.

int parse_file(cobalt_ctx* ctx) {
    //load file into buffer
    FsFile* curr_file = fs_open(clone_to_cstring(ctx->curr_file), false, false);
    if (curr_file == NULL) {
        printf("unable to open file: "str_fmt"\n", str_arg(ctx->curr_file));
        return -1;
    }

    string buf = string_alloc(curr_file->size);
    fs_read(curr_file, buf.raw, buf.len);
    //now, we split the entire file up into "lines"
    //this forms the physical lines, which will be augmented to then form the logical lines
    //__LINE__ does not respect physical line information, so for ease of implementation we'll make it respect
    //logical line information.
    Vec(string) physical_lines = vec_new(string, 1);
    size_t starting_val = 0;
    size_t len = 0;

    for (size_t i = 0; i < buf.len; i++) {
        //even though the c standard says in 5.1.1.2.2 that 
        //"A source file that is not empty shall end in a new-line character, which shall not be immediately preceded by a
        //backslash character before any such splicing takes place."
        //we're gonna allow non-newline ends
        if (buf.raw[i] == '\n' || i + 1 >= buf.len) {
            //we've hit a newline (or EOF)!
            //increase len, and then slice
            if (!(i + 1 >= buf.len)) len++;
            //we create a new line allocation and put it in physical_lines
            //since we wanna mutate these lines to put them in logical_lines
            //could this be optimised to use less memory? yeah, but thats fine
            string line = string_alloc(len);
            //move info into line
            memmove(line.raw, buf.raw + starting_val, len);
            //append and update walker info
            vec_append(&physical_lines, line);
            starting_val = i + 1;
            len = 0;
            continue;
        }
        len++;
    }

    //phase 1 is skipped for now
    //parser_phase1(ctx);

    parser_ctx pctx = (parser_ctx){.tokens = vec_new(token, 1),
                                   .curr_offset = 0,
                                   .ctx = ctx};

    if (parser_phase2(&pctx, physical_lines) != 0) return -1;

    if (parser_phase3(&pctx) != 0) return -1;

    return 0;
}

void parser_phase1(cobalt_ctx* ctx) {

}

int parser_phase2(parser_ctx* ctx, Vec(string) physical_lines) {
    //phase 2
    //find all instances of \\n, and delete them, creating fresh new logical lines
    //we also trim \n here in our logical lines, breaking phase 3 conformance, but its fine
    Vec(string) logical_lines = vec_new(string, 1);

    for_vec(string* line, &physical_lines) {
        //scan to the end of the line to see if it matches the pattern "\\n"
        //if the line is the LAST line, this check should error!
        string mut_line = *line;
        //same check as the .c check from main.c
        mut_line.raw = mut_line.raw + mut_line.len - 2;
        mut_line.len = 2;
        if (string_eq(mut_line, constr("\\\n"))) {
            //we need to merge lines!
            //first, check if this is the last line
            if (_index + 1 > physical_lines.len) {
                //we're at the end! this is ERRORING!
                printf("violation of 5.1.1.2.2: A source file that is not empty shall end in a new-line character, which shall not be immediately preceded by a backslash character before any such splicing takes place.\n");
                return -1;
            }
            //get rid of the \\n
            line->len -= 2;
            //create new string with next one appended, then make it logical
            string new_line = string_concat(*line, physical_lines.at[_index + 1]);
            new_line.len--; //fix newline
            vec_append(&logical_lines, new_line);
            //skip next string, we've already handled it
            _index++;
            continue;
        }

        line->len--; //delete newline
        vec_append(&logical_lines, *line);
    }
    ctx->logical_lines = logical_lines;
    return 0;
}

// <pp-identifier> ::= <non_digit> (<non_digit> | <digit> | "_")*
// we ignore XID_Start and XID_Continue characteristics, since we dont support wchar.

#define CURR_LINE ctx->logical_lines.at[ctx->curr_line]

int pp_scan_identifier(parser_ctx* ctx) {
    u8 len = 0;
    u8 offset = ctx->curr_offset;
    u8 c = CURR_LINE.raw[ctx->curr_offset];
    if (isalpha(c) || c == '_') {
        len++;
    } else {
        printf("expected [a-zA-Z], got %c\n", c);
        return -1;
    }

    for (size_t i = ctx->curr_offset + 1; i < CURR_LINE.len; i++) {
        c = CURR_LINE.raw[i];
        // (<non_digit> | <digit> | "_")*
        if (isalnum(c) || c == '_') {
            len++;
            continue;
        } 
        break;
    }

    token new_tok = (token){.type = TOK_IDENTIFIER,
                            .tok = string_make(CURR_LINE.raw + offset, len),
                            .line = ctx->curr_line};
    vec_append(&ctx->tokens, new_tok);
    ctx->curr_offset += len;
    return 0;
}

/*
<pp-token> ::= <header-name> | 
               <pp-identifier> |
               <character-constant> |
               <pp-string-literal> |
               <pp-punctuator> |
               "\u" <hex-quad> (not supported)
*/

#define scan_next_char() \
    ((ctx->curr_offset + 1 < line->len) ? (line->raw[ctx->curr_offset + 1]) : ('\0'))

#define CURR_CHAR line->raw[ctx->curr_offset]

int parser_phase3(parser_ctx* ctx) {
    /* to scan a token, we check:
    < means header-name or punct
    " means header-name OR string literal, decided when scanning for q-char
    u8', u', U' or L' means char constant
    all others are punctuators, or produce a syntax error
    this means we need to check things BEFORE we scan
    */

    //we continually iterate over all the characters in each logical line,
    //and if we come across special characters, we operate inside this switch case to get
    //correct tokenisation behaviour.
    //no pp-toks require us to keep track of whitespace, so we're ignoring
    //whitespace rules.

    bool ml_comment = false;
    bool is_header = false;
    for_vec(string* line, &ctx->logical_lines) {
        ctx->curr_line = _index; //for pp_scan_identifier
        for (ctx->curr_offset = 0; ctx->curr_offset < line->len; ctx->curr_offset++) {
            if (ml_comment == true) {
                //we are in ml comment scanning mode.
                //we scan this char, and the next char, until we find */. we can then exit ml_comment mode
                if (CURR_CHAR == '*' && scan_next_char() == '/') {
                    ml_comment = false;
                    ctx->curr_offset++;
                }
                continue;
            }
            switch (CURR_CHAR) {
                case ' ':
                case '\t':
                case '\n':
                    continue; //skip trivial whitespace
                case '/': {
                    //this COULD be punctuation, namely / or /=. we now check for this
                    if (scan_next_char() != '=' && scan_next_char() != '*' && scan_next_char() != '/') { // / case.
                        //this if is kinda ugly, but its required to correctly lex with identifiers against punct
                        token new_tok = (token){.type = PPTOK_PUNCT,
                                                .tok = string_make(line->raw + ctx->curr_offset, 1),
                                                .line = ctx->curr_line};
                        vec_append(&ctx->tokens, new_tok);
                        //we dont skip offset here, since we only scan this one
                        continue;
                    } else if (scan_next_char() == '=') { // /= case
                        token new_tok = (token){.type = PPTOK_PUNCT,
                                                .tok = string_make(line->raw + ctx->curr_offset, 2),
                                                .line = ctx->curr_line};
                        vec_append(&ctx->tokens, new_tok);
                        ctx->curr_offset++;
                        continue;                        
                    } else if (scan_next_char() == '*') { // /* case
                        //this one is VERY special.
                        //we now scan ahead, searching every 2 characters for a corresponding */
                        //this requires a lexer state change
                        ctx->curr_offset++; //skip /*
                        ml_comment = true;
                        continue;  
                    } else if (scan_next_char() == '/') { // // case
                        //we can now skip this line COMPLETELY, so we set offset to the end of the line
                        ctx->curr_offset = line->len;
                        continue;
                    } else {
                        printf("expected /, // comment, /= or /*\n");
                        return -1;
                    }
                }
                default:
                    printf("encountered unexpected char %c when lexing %d:%d\n", line->raw[ctx->curr_offset], _index + 1, ctx->curr_offset + 1);
                    return -1;
            }
        }
    }

    if (ml_comment == true) {
        printf("did not find a corresponding */ to close a multi-line comment.\n");
    }


}
