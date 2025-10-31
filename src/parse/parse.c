#include <ctype.h>
#include <unistd.h>

#include "orbit.h"
#include "cobalt.h"
#include "parse.h"
#include "ansi.h"
#include "str.h"

// we will NOT enforce translation limits in this compiler, as per C23 5.2.5.2 footnote 13.
// NOTABLE deviations: we ignore 6.10.5.4.3, since that seems fucking annoying. if this comes up as an issue,
//                     we can implement this correctly.                 

int parse_file(cobalt_ctx* ctx, bool is_include) {
    //load file into buffer
    FsFile* curr_file = fs_open(clone_to_cstring(ctx->curr_file), false, false);
    if (curr_file == NULL && !is_include) {
        printf("unable to open file "str_fmt": %s\n", str_arg(ctx->curr_file), strerror(errno));
        return -1;
    } else if (curr_file == NULL) {
        //we search in the ctx's include paths for the correct path.
        for_vec(string* path, &ctx->include_paths) {
            string new_path = string_concat(*path, ctx->curr_file);
            printf("trying path: "str_fmt"\n", str_arg(new_path));
            curr_file = fs_open(clone_to_cstring(new_path), false, false);
            if (curr_file != NULL) {
                _index = ctx->include_paths.len + 1;
            }
        }

        if (curr_file == NULL) {
            printf("unable to open file2: "str_fmt"\n", str_arg(ctx->curr_file));
            return -1;
        }
    }



    string buf = string_alloc(curr_file->size + 1);
    fs_read(curr_file, buf.raw, buf.len);
    fs_close(curr_file);

    #ifdef FUZZ
    unlink("fuzz.c");
    #endif

    //now, we split the entire file up into "lines"
    //this forms the physical lines, which will be augmented to then form the logical lines
    //__LINE__ does not respect physical line information, so for ease of implementation we'll make it respect
    //logical line information.
    Vec(string) physical_lines = vec_new(string, 1);
    size_t starting_val = 0;
    size_t len = 0;

    //TODO FATAL: fix the off by one here thats causing an OOB read w/ memcpy
    for (size_t i = 0; i < buf.len; i++) {
        //even though the c standard says in 5.1.1.2.2 that 
        //"A source file that is not empty shall end in a new-line character, which shall not be immediately preceded by a
        //backslash character before any such splicing takes place."
        //we're gonna allow non-newline ends
        if (buf.raw[i] == '\n' || i + 1 >= buf.len) {
            if (len == 0) {
                starting_val++;
                continue;
            }
            //we've hit a newline (or EOF)!
            //increase len, and then slice
            
            len++;
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

    parser_ctx* pctx = cmalloc(sizeof(*pctx));
    *pctx = (parser_ctx){.tokens = vec_new(token, 1),
                         .curr_offset = 0,
                         .ctx = ctx,
                         .pragma_files = vec_new(string, 1),
                         .defines = vec_new(macro_define, 1)};

    ctx->pctx = pctx;

    if (parser_phase2(pctx, physical_lines) != 0) return -1;

    if (parser_phase3(pctx) != 0) return -1;

    int retval = parser_phase4(pctx);
    if (retval == 0x1234) return 0x1234; //pragma sentinel
    else if (retval != 0) return -1;
    
    if (is_include) return 0;

    if (parser_phase5(pctx) != 0) return -1;

    if (parser_phase6(pctx) != 0) return -1;

    print_token_stream(pctx);

    cfree(pctx);

    return 0;
}

void print_parsing_error(parser_ctx* ctx, token err_tok, char* format, ...) {
    return;
    //this handles errors relating to tokens, and so needs a token based error printing
    print_token_stream(ctx);
    if (ctx->ctx->no_colour) printf(str_fmt ":%d: error: ", str_arg(ctx->ctx->curr_file), err_tok.line + 1);
    else printf(Bold str_fmt ":%d: " Reset Red Bold"error: "Reset, str_arg(ctx->ctx->curr_file), err_tok.line + 1);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
    
    //left justify the number when printing
    size_t num_len = snprintf(NULL, 0, "%d", err_tok.line + 1);
    string left_just_string = str("     ");
    left_just_string.raw += num_len;
    printf(str_fmt"%d | ", str_arg(left_just_string), err_tok.line + 1);

    //assuming this token is actually from the line we care about, this should be relatively easy.
 
    //split the erroring line into 3 pieces, so we can bold the section we want
    string error_line = ctx->logical_lines.at[err_tok.line];
    string left_piece = string_make(error_line.raw, err_tok.tok.raw - error_line.raw);
    string central_piece = err_tok.tok;
    string right_piece = string_make(error_line.raw + left_piece.len + err_tok.tok.len, error_line.len - central_piece.len - left_piece.len);
    
    if (left_piece.len + central_piece.len + 1 > 0xFFFF) return; //we're in a macro

    //print out the erroring line
    if (ctx->ctx->no_colour) printf(str_fmt"\n", str_arg(error_line));
    else printf(str_fmt Bold Red str_fmt Reset str_fmt, str_arg(left_piece), str_arg(central_piece), str_arg(right_piece));

    if (right_piece.raw[right_piece.len - 1] != '\n') printf("\n");

    //on linux, we could use ansi escape sequences to move the cursor.
    //i do not trust microsoft to implement this correctly.
    string empty_space;
    empty_space.raw = ccharalloc(left_piece.len + central_piece.len + 1, ' ');
    empty_space.len = left_piece.len + central_piece.len + 1;

    //now, we scan left_piece until we hit non-whitespace
    size_t i = 0;
    for (; i < left_piece.len; i++) {
        if (isgraph(left_piece.raw[i])) break;
    }
    //copy that many bytes from left_piece into empty_space, so that the tabs match to ensure correct positioning
    memcpy(empty_space.raw + 1, left_piece.raw, i);
    //now, we fill out with ~
    for (size_t i = left_piece.len; i < left_piece.len + central_piece.len; i++) {
        empty_space.raw[i] = '~';
    }
    empty_space.raw[left_piece.len] = '^';

    if (ctx->ctx->no_colour) printf("      | "str_fmt"\n", str_arg(empty_space));
    else printf("      | "Red Bold str_fmt Reset"\n", str_arg(empty_space));
    return;
}

void print_parsing_warning(parser_ctx* ctx, token err_tok, char* format, ...) {
    //this handles errors relating to tokens, and so needs a token based error printing

    if (ctx->ctx->no_colour) printf(str_fmt ":%d: warning: ", str_arg(ctx->ctx->curr_file), err_tok.line + 1);
    else printf(Bold str_fmt ":%d: " Yellow Bold"warning: "Reset, str_arg(ctx->ctx->curr_file), err_tok.line + 1);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
    
    //left justify the number when printing
    size_t num_len = snprintf(NULL, 0, "%d", err_tok.line + 1);
    string left_just_string = str("     ");
    left_just_string.raw += num_len;
    printf(str_fmt"%d | ", str_arg(left_just_string), err_tok.line + 1);

    //assuming this token is actually from the line we care about, this should be relatively easy.

    //split the erroring line into 3 pieces, so we can bold the section we want
    string error_line = ctx->logical_lines.at[err_tok.line];
    string left_piece = string_make(error_line.raw, err_tok.tok.raw - error_line.raw);
    string central_piece = err_tok.tok;
    string right_piece = string_make(error_line.raw + left_piece.len + err_tok.tok.len, error_line.len - central_piece.len - left_piece.len);
    
    //print out the erroring line
    if (ctx->ctx->no_colour) printf(str_fmt"\n", str_arg(error_line));
    else printf(str_fmt Bold Yellow str_fmt Reset str_fmt, str_arg(left_piece), str_arg(central_piece), str_arg(right_piece));

    if (right_piece.raw[right_piece.len - 1] != '\n') printf("\n");

    //on linux, we could use ansi escape sequences to move the cursor.
    //i do not trust microsoft to implement this correctly.
    string empty_space;
    empty_space.raw = ccharalloc(left_piece.len + central_piece.len + 1, ' ');
    empty_space.len = left_piece.len + central_piece.len + 1;

    //now, we scan left_piece until we hit non-whitespace
    size_t i = 0;
    for (; i < left_piece.len; i++) {
        if (isgraph(left_piece.raw[i])) break;
    }
    //copy that many bytes from left_piece into empty_space, so that the tabs match to ensure correct positioning
    memcpy(empty_space.raw + 1, left_piece.raw, i);
    //now, we fill out with ~
    for (size_t i = left_piece.len; i < left_piece.len + central_piece.len; i++) {
        empty_space.raw[i] = '~';
    }
    empty_space.raw[left_piece.len] = '^';

    if (ctx->ctx->no_colour) printf("      | "str_fmt"\n", str_arg(empty_space));
    else printf("      | "Yellow Bold str_fmt Reset"\n", str_arg(empty_space));
    return;
}

macro_define init_define() {
    macro_define new_def = (macro_define){.is_function = false,
                                          .arguments = vec_new(token, 1),
                                          .replacement_list = vec_new(token, 1)};
    return new_def;
}

void print_token_stream(parser_ctx* ctx) {
    size_t curr_line = 0;
    printf("0: ");
    for_vec(token* tok, &ctx->tokens) {
        if (tok->line != curr_line) {
            curr_line = tok->line;
            printf("\n%d: ", curr_line);
        }
        printf(str_fmt, str_arg(tok->tok));
    }
    printf("\n");
}

void parser_phase1(cobalt_ctx* ctx) {

}

int parser_phase2(parser_ctx* ctx, Vec(string) physical_lines) {
    //phase 2
    //find all instances of \\n, and delete them, creating fresh new logical lines
    //we also trim \n here in our logical lines, breaking phase 3 conformance, but its fine
    Vec(string) logical_lines = vec_new(string, 1);

    string new_line = {.raw = NULL, .len = 0};
    for_vec(string* line, &physical_lines) {
        if (new_line.raw == NULL) new_line = *line;
        //scan to the end of the line to see if it matches the pattern "\\n"
        //if the line is the LAST line, this check should error!
        string mut_line = new_line;
        //same check as the .c check from main.c
        mut_line.raw = mut_line.raw + mut_line.len - 2;
        mut_line.len = 2;

        if (string_eq(mut_line, constr("\\\n"))) {
            //we need to merge lines!
            //first, check if this is the last line
            if (_index + 1 >= physical_lines.len) {
                //we're at the end! this is ERRORING!
                print_lexing_error(ctx, "Unexpected \\ at end of file");
                return -1;
            }
            //get rid of the \\n
            new_line.len -= 2;
            //create new string with next one appended, then make it logical
            new_line = string_concat(new_line, physical_lines.at[_index + 1]);
            
            continue;
        } else {
            new_line.len--;
            vec_append(&logical_lines, new_line);
            new_line = (string){.raw = NULL, .len = 0};
        }
    }
    ctx->logical_lines = logical_lines;

    return 0;
}

/*  Each source character set member and escape sequence in character constants and string
    literals is converted to the corresponding member of the execution character set. Each instance
    of a source character or escape sequence for which there is no corresponding member is
    converted in an implementation-defined manner to some member of the execution character
    set other than the null (wide) character.
*/
int parser_phase5(parser_ctx* ctx) {
    return 0;
}

extern char* token_str[];

#define skip_token(offset) do { \
    ctx->curr_tok_index += offset; \
} while(0)

#define next_token() ((ctx->curr_tok_index + 1 < ctx->tokens.len) ? ctx->tokens.at[ctx->curr_tok_index + 1] : (token){})

#define next_token_from(offset) ((ctx->curr_tok_index + (offset) < ctx->tokens.len) ? ctx->tokens.at[ctx->curr_tok_index + (offset)] : (token){})

#define curr_token() ((ctx->curr_tok_index < ctx->tokens.len) ? ctx->tokens.at[ctx->curr_tok_index] : (token){})

#define skip_whitespace() do { \
    if (curr_token().type == TOK_WHITESPACE) ctx->curr_tok_index++; \
} while(0)

// Adjacent string literal tokens are concatenated.
int parser_phase6(parser_ctx* ctx) {
    for_vec(token* tok, &ctx->tokens) {
        //augh.
        ctx->curr_tok_index = _index;
        size_t old_index = _index;
        if (tok->type == PPTOK_STR_LIT) {
            skip_token(1);
            if (curr_token().type == TOK_WHITESPACE) skip_token(1);
            if (curr_token().type != PPTOK_STR_LIT) continue;

            
            //we combine the two strlits
            token left_str = *tok;
            token right_str = curr_token();
            //cut off extraneous " chars
            left_str.tok.len -= 1;
            right_str.tok.len -= 1;
            right_str.tok.raw += 1;

            string new_strlit = string_concat(left_str.tok, right_str.tok);
            token new_str = {.type = PPTOK_STR_LIT,
                             .itype = TOK_STR_LIT,
                             .line = left_str.line,
                             .was_included = false,
                             .tok = new_strlit};
            //remove left and right strings
            vec_remove(&ctx->tokens, old_index);
            if (ctx->tokens.at[old_index].type == TOK_WHITESPACE) vec_remove(&ctx->tokens, old_index);
            vec_remove(&ctx->tokens, old_index);
            
            //insert new string
            vec_insert(&ctx->tokens, old_index, new_str);
            
            //restore _index
            _index = old_index - 1;
        }
    }
    return 0;
}

int parser_phase7(parser_ctx* ctx) {
    
}