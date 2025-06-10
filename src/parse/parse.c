#include <ctype.h>

#include "orbit.h"
#include "cobalt.h"
#include "parse.h"
#include "ansi.h"

char* token_str[] = {
#define TOKEN(tok, str) str,
    TOKENS
#undef TOKEN
};

// we will NOT enforce translation limits in this compiler, as per C23 5.2.5.2 footnote 13.
// NOTABLE deviations: we ignore 6.10.5.4.3, since that seems fucking annoying. if this comes up as an issue,
//                     we can implement this correctly.                 


int parse_file(cobalt_ctx* ctx, bool is_include) {
    //load file into buffer
    FsFile* curr_file = fs_open(clone_to_cstring(ctx->curr_file), false, false);
    if (curr_file == NULL && !is_include) {
        printf("unable to open file: "str_fmt"\n", str_arg(ctx->curr_file));
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
    //parser_phase3(pctx);
    //print_token_stream(pctx);

    int retval = parser_phase4(pctx);
    if (retval == 0x1234) return 0x1234; //pragma sentinel
    else if (retval != 0) return -1;
    

    if (is_include) return 0;

    print_token_stream(pctx);

    cfree(pctx);

    return 0;
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
            if (_index + 1 > physical_lines.len) {
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

void print_parsing_error(parser_ctx* ctx, token err_tok, char* format, ...) {
    //this handles errors relating to tokens, and so needs a token based error printing

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

string pp_stringize_token_stream(Vec(token) stream) {
    string builder = string_alloc(64);
    size_t cursor = 1;
    builder.raw[0] = '\"';
    for_vec(token* tok, &stream) {
        if (cursor + tok->tok.len > builder.len) {
            //we need to realloc the raw builder's value. we double it and add the token's length
            builder.raw = crealloc(builder.raw, builder.len * 2 + tok->tok.len);
            builder.len = builder.len * 2 + tok->tok.len;
        }
        
        if (tok->type == PPTOK_STR_LIT) {
            //this one is gonna be fun.
            memmove(builder.raw + cursor, "\\\"", 2);
            cursor += 2;
            memmove(builder.raw + cursor, tok->tok.raw + 1, tok->tok.len - 2);
            cursor += tok->tok.len - 2;
            memmove(builder.raw + cursor, "\\\"", 2);
            cursor += 2;
            continue;
        }

        if (tok->type == TOK_WHITESPACE) {
            memmove(builder.raw + cursor, " ", 1);
            cursor++;
            continue;
        }

        memmove(builder.raw + cursor, tok->tok.raw, tok->tok.len);
        cursor += tok->tok.len;
    }
    //now, we add a " to the end
    if (cursor + 1 > builder.len) {
        builder.raw = crealloc(builder.raw, builder.len + 1);
        builder.len++;
    }
    builder.raw[cursor] = '\"';
    //we now need to fix the length
    builder.len = cursor + 1;

    return builder;
}

int pp_replace_ident(parser_ctx* ctx, size_t index) {
    //scan macro defines list, if macro is defined we can cut it off
    if (index > ctx->tokens.len) crash("Attempted to replace identifier thats not in the ctx->tokens list!\n");
    if (ctx->tokens.at[index].type != PPTOK_IDENTIFIER) crash("Attempted to replace non-identifier! Got: %s\n", token_str[ctx->tokens.at[index].type]);
    //we need to recursively call pp_replace_ident for any identifiers that do NOT match this one
    macro_define potential_define = {};
    bool found_define = false;

    printf("expanding "str_fmt"\n", str_arg(ctx->tokens.at[index].tok));
    print_token_stream(ctx);    
    token replaced_tok = ctx->tokens.at[index];

    //find the correct macro
    for_vec(macro_define* define, &ctx->defines) {
        if (string_eq(replaced_tok.tok, define->name.tok)) {
            found_define = true;
            potential_define = *define;
            break;
        }
    }

    if (found_define == false) return 0;
    
    if (potential_define.is_function == false) {
        //now, we delete the token at index
        vec_remove(&ctx->tokens, index);
        //and repeatedly add the tokens in the list
        for_vec(token* tok, &potential_define.replacement_list) {
            token new_tok = *tok;
            //we need to make sure we update line info correctly
            new_tok.line = replaced_tok.line;
            vec_insert(&ctx->tokens, index + _index, new_tok);
        }
    } else {
        //we scan ahead until we hit (, and then we grab the arguments
        size_t tok_cursor = index;
        tok_cursor++; //skip identifier
        //printf("%d, %d\n", tok_cursor, ctx->tokens.len);
        if (tok_cursor >= ctx->tokens.len) {
            printf("%d, %d\n", tok_cursor, ctx->tokens.len);
        
            printf("encountered oob tok_cursor when parsing args for "str_fmt"\n", str_arg(replaced_tok.tok));
            return 0;
        }
        if (ctx->tokens.at[tok_cursor].type == TOK_WHITESPACE) tok_cursor++;
        if (ctx->tokens.at[tok_cursor].itype != CTOK_OPEN_PAREN) return 0;
        //we're doing some non-standard jank here, since nested Vec isnt possible with current
        //orbit vec.
        
        //FIXME?: handle when args are missing, but commas are there. basically, create and insert a nothing token on the same line
        Vec(_Vec_token) args = vec_new(Vec(token), 1);
        tok_cursor++;
        Vec(token) arg_list = vec_new(token, 1);
        size_t curr_depth = 0;
        for (; tok_cursor < ctx->tokens.len; tok_cursor++) {
            //if (ctx->tokens.at[tok_cursor].type == TOK_WHITESPACE) continue;

            if (ctx->tokens.at[tok_cursor].itype == CTOK_OPEN_PAREN) {
                vec_append(&arg_list, ctx->tokens.at[tok_cursor]);
                curr_depth++;
                continue;
            }

            if (ctx->tokens.at[tok_cursor].itype == CTOK_CLOSE_PAREN) {
                if (curr_depth != 0) {
                    vec_append(&arg_list, ctx->tokens.at[tok_cursor]);
                    curr_depth--;
                    continue;
                }
                //if args are empty, we skip appending.

                if (arg_list.len == 0) break;

                vec_append(&args, arg_list);
                break;
            }
            if (ctx->tokens.at[tok_cursor].itype == CTOK_COMMA) {
                //expect no close paren after this
                if (tok_cursor + 1 < ctx->tokens.len && ctx->tokens.at[tok_cursor + 1].itype == CTOK_CLOSE_PAREN) {
                    //AUGH!
                    //normally, this would be an issue, but we'll simply append the args to the arglist, and continue
                    vec_append(&args, arg_list);
                    arg_list = vec_new(token, 1);

                    string fake_str = string_alloc(1);
                    fake_str.len = 0;
                    token fake_tok = (token){.type = TOK_WHITESPACE,
                                             .tok = fake_str,
                                             .line = ctx->tokens.at[tok_cursor].line};
                    vec_append(&arg_list, fake_tok);
                    vec_append(&args, arg_list);
                    continue;
                }
                if (arg_list.len == 0) {
                    //we need to create a "nothing" token here, so that we can insert an empty something there
                    string fake_str = string_alloc(1);
                    fake_str.len = 0;
                    token fake_tok = (token){.type = TOK_WHITESPACE,
                                             .tok = fake_str,
                                             .line = ctx->tokens.at[tok_cursor].line};
                    vec_append(&arg_list, fake_tok);
                }
                if (potential_define.is_variadic && args.len == potential_define.arguments.len - 1) {
                    //we safely ignore this comma.
                    vec_append(&arg_list, ctx->tokens.at[tok_cursor]);
                    continue;
                }

                vec_append(&args, arg_list);
                arg_list = vec_new(token, 1);
                if (ctx->tokens.at[tok_cursor].type == TOK_WHITESPACE) tok_cursor++;
                continue;
            }
            vec_append(&arg_list, ctx->tokens.at[tok_cursor]);
        }
        //now that we've gotten the argument list, we can start matching 1:1
        //only problem: we have to replace in place, so when we insert a token, we need to recursively call pp_replace_ident
        //first, verify the expansion is actually valid
        if (potential_define.is_variadic == false && args.len != potential_define.arguments.len) {
            print_token_stream(ctx);
            print_parsing_error(ctx, replaced_tok, "macro "str_fmt" takes %d args, given %d", str_arg(replaced_tok.tok), potential_define.arguments.len, args.len);
            return -1;
        }

        if (potential_define.is_variadic) {
            //we need to count the number of args, and if they're less than the minimum required, we error
            //e.g
            //#define some_var(a, b, ...) a, b, __VA_ARGS__
            //and we use it like
            //some_var(1)
            //thats not enough args
            size_t non_var_count = 0;
            for_vec(token* tok, &potential_define.arguments) {
                if (tok->itype == CTOK_ELLIPSIS) break;
                non_var_count++;
            }
            if (args.len < non_var_count) {
                print_parsing_error(ctx, replaced_tok, "variadic macro "str_fmt" takes at least %d args, given %d", str_arg(replaced_tok.tok), non_var_count, args.len);
                return -1;
            }
        }
        //now that we know everything is fine, we can start inserting
        //we split this into two paths, function and var function

        //index contains the name of the macro we started at, so we can just start replacing here
        //we need to recursively expand macros internally, which is going to get.. messy
        //so while we're scanning the replacement list, once we've replaced, we then check to see if we need to run pp_replace_ident
        //we now remove tokens until we hit the final close paren
        curr_depth = 0;
        //we delete the identifier
        for (; index < ctx->tokens.len;) {
            if (ctx->tokens.at[index].itype == CTOK_OPEN_PAREN) {
                vec_remove(&ctx->tokens, index);
                curr_depth++;
                continue;
            }

            if (ctx->tokens.at[index].itype == CTOK_CLOSE_PAREN) {
                if (curr_depth != 1) {
                    curr_depth--;
                    vec_remove(&ctx->tokens, index);
                    continue;
                }
                vec_remove(&ctx->tokens, index);
                break;
            }
            vec_remove(&ctx->tokens, index);
        }

        //we've deleted the macro, now we need to fill it back out with info
        //first, we shove the replacement list into a special vec.
        Vec(token) replacement_list = vec_new(token, 1);
        for_vec(token* tok, &potential_define.replacement_list) {
            token new_tok = *tok;
            new_tok.line = replaced_tok.line;
            vec_append(&replacement_list, new_tok);
        }

        //now, scan the replacement list. if we find a #, we need to deal with it
        for_vec(token* tok, &replacement_list) {
            if (tok->itype == CTOK_HASH) {
                //stringise the whole thing that comes next
                //first, is it an argument?
                _index++;
                if (_index > replacement_list.len) break;
                //is arg[i] == next token?
                for (size_t i = 0; i < potential_define.arguments.len; i++) {
                    if (string_eq(potential_define.arguments.at[i].tok, replacement_list.at[_index].tok)) {
                        //we have an argument. get the index, and then stringize the whole token sequence
                        Vec(token) arg_tokens = args.at[i];
                        string stringised = pp_stringize_token_stream(arg_tokens);
                        //now we have the stringised stream, we need to create a new token
                        token str_tok = (token){.type = PPTOK_STR_LIT,
                                                .tok = stringised,
                                                .line = replaced_tok.line,
                                                .from_macro_param = true};
                        vec_insert(&replacement_list, _index - 1, str_tok);
                        //we delete the two tokens that caused this
                        vec_remove(&replacement_list, _index);
                        vec_remove(&replacement_list, _index);
                        
                        _index--;
                        break;
                    }
                }
                continue;
            }
        }

        //then, we scan again, looking for identifiers to replace.
        for_vec(token* tok, &replacement_list) {
            //printf("rl size: %d\n", replacement_list.len);
            if (tok->type == PPTOK_IDENTIFIER) {
                //this COULD be an argument. if it is, we replace it. otherwise, we just insert a normal token
                if (_index > replacement_list.len) break;
                //is arg[i] == next token?
                for (size_t i = 0; i < potential_define.arguments.len; i++) {
                    if (string_eq(potential_define.arguments.at[i].tok, tok->tok) && tok->from_macro_param == false) {
                        //we have an argument. get the index, and then repeatedly insert.
                        Vec(token) arg_tokens = args.at[i];
                        //remove the identifier that caused this
                        vec_remove(&replacement_list, _index);

                        for (size_t j = 0; j < arg_tokens.len; j++) {
                            token inserted_tok = arg_tokens.at[j];
                            inserted_tok.from_macro_param = true;
                            vec_insert(&replacement_list, j + _index, inserted_tok);
                        }
                        break;
                    }

                    else if (string_eq(tok->tok, constr("__VA_ARGS__"))) {
                        //we've got __VA_ARGS__
                        //go to the index given by ... in the args list
                        //if theres nothing there, we dont insert anything.

                        vec_remove(&replacement_list, _index);  
                        if (potential_define.arguments.len - 1 == args.len) break;

                        Vec(token) arg_tokens = args.at[args.len - 1];

                        for (size_t j = 0; j < arg_tokens.len; j++) {
                            token inserted_tok = arg_tokens.at[j];
                            inserted_tok.from_macro_param = true;
                            vec_insert(&replacement_list, j + _index, inserted_tok);
                        }
                        break;                        
                    }
                    else if (string_eq(tok->tok, constr("__VA_OPT__"))) {
                        print_parsing_error(ctx, *tok, "__VA_OPT__ not supported");
                        return -1;
                    }
                }
                continue;
            }
        }
        //now, we process concatenations
        for_vec(token* tok, &replacement_list) {
            //this works specially, because we need to scan _ahead_ to HASH_HASH
            //we skip whitespace between
            if (tok->type == TOK_WHITESPACE) continue;
            if (tok->itype != CTOK_HASH_HASH) {
                bool is_concat = false;
                size_t num_tok = _index + 1; // we need to know how many tokens to destroy from _index
                for (; num_tok < replacement_list.len; num_tok++) {
                    token curr_tok = replacement_list.at[num_tok];
                    if (curr_tok.itype == CTOK_HASH_HASH) {
                        is_concat = true;
                        continue;
                    }
                    if (curr_tok.type == TOK_WHITESPACE) continue;
                    num_tok++;
                    break;
                }
                if (!is_concat) continue;

                token left = *tok;
                token right = replacement_list.at[num_tok - 1];
                //now, we clear the tokens
                for (size_t i = _index; i < num_tok; i++) {
                    vec_remove(&replacement_list, _index);
                }

                //we can now create and check our concated token
                //this will be tricky, since we need to detect that this is a valid symbol, and assign it an itype and a type.
                //for now, we ignore these.
                //lord, forgive me for what i'm about to do.
                //since we dont have a consumption parser, we cant throw arbitrary tokens at something to verify their type.
                //we instead need to fake a lexing step, so we pass a rudimentary context into phase 3, and then steal the
                //token back at the end.
                parser_ctx fake_ctx = {.logical_lines = vec_new(string, 1),
                                       .ctx = ctx->ctx,
                                       .tokens = vec_new(token, 1)};
                vec_append(&fake_ctx.logical_lines, string_concat(left.tok, right.tok));

                parser_phase3(&fake_ctx);
                //steal the token back. we maintain ownership either way
                token stolen_tok = fake_ctx.tokens.at[0];

                token new_tok = (token){.tok = stolen_tok.tok,
                                        .type = stolen_tok.type,
                                        .itype = stolen_tok.itype,
                                        .line = replaced_tok.line,
                                        .from_macro_param = false};
                vec_insert(&replacement_list, _index, new_tok);
                continue;
            }
        }

        //we can now expand tokens that originated from this process, by creating a fake pctx
        //we scan left to right to do this
        for_vec_bw(token* tok, &replacement_list) {
            parser_ctx temp_ctx = {.tokens = replacement_list,
                                   .logical_lines = ctx->logical_lines,
                                   .ctx = ctx->ctx,
                                   .defines = ctx->defines,
                                   .curr_macro_name = *tok};
            if (tok->type == PPTOK_IDENTIFIER && tok->from_macro_param == true) {
                if (string_eq(tok->tok, ctx->curr_macro_name.tok)) continue;
                pp_replace_ident(&temp_ctx, _index - 1);
                //we need to update replacement_list's len info, since it got destroyed during recursive replacement
                //we also need to grab the ctx's pointer, since it could have changed.
                replacement_list.len = temp_ctx.tokens.len;
                replacement_list.at = temp_ctx.tokens.at;
            }
        }

        //now, we can expand any OTHER tokens.
        for_vec_bw(token* tok, &replacement_list) {
            parser_ctx temp_ctx = {.tokens = replacement_list,
                                   .logical_lines = ctx->logical_lines,
                                   .ctx = ctx->ctx,
                                   .defines = ctx->defines,
                                   .curr_macro_name = *tok};
            if (tok->type == PPTOK_IDENTIFIER) {
                if (string_eq(tok->tok, ctx->curr_macro_name.tok)) continue;
                pp_replace_ident(&temp_ctx, _index - 1);
                //we need to update replacement_list's len info, since it got destroyed during recursive replacement
                //we also need to grab the ctx's pointer, since it could have changed.
                replacement_list.len = temp_ctx.tokens.len;
                replacement_list.at = temp_ctx.tokens.at;
            }                
        }

        //finally, we put our replacement list into the source
        for_vec(token* tok, &replacement_list) {
            vec_insert(&ctx->tokens, index + _index, *tok);
        }

        //print_token_stream(ctx);
        //return -1;

    }
    //print_token_stream(ctx);
    return 0;
}

#define next_token() ((_index + 1 < ctx->tokens.len) ? ctx->tokens.at[_index + 1] : (token){})

#define next_token_from(offset) ((_index + (offset) < ctx->tokens.len) ? ctx->tokens.at[_index + (offset)] : (token){})

#define curr_token() ((_index < ctx->tokens.len) ? ctx->tokens.at[_index] : (token){})

#define skip_whitespace() do { \
    if (curr_token().type == TOK_WHITESPACE) _index++; \
} while(0)

int parser_phase4(parser_ctx* ctx) {
    //phase 4 is macro replacement, and also any relevant cleanup from phase 3, along with any error catching
    //this means we're now doing errors, like for real this time

    //we dont ever actually use header names, since we never lex them properly
    //either way, its directin time
    
    for_vec(token* tok, &ctx->tokens) {
        if (tok->itype == CTOK_HASH && tok->after_newline == true) {
            size_t hash_location = _index;
            _index++;
            //we're in business, got a directive
            //next, scan for the type
            //first, check for ws
            if (curr_token().type == TOK_WHITESPACE) _index++;
            if (string_eq(curr_token().tok, constr("include"))) {
                //we've got an include!
                //now, we need to skip the whitespace, and get onto the include.
                _index+=2;
                if (curr_token().type == TOK_WHITESPACE) _index++;
                //now, we need to get onto the header itself
                //if we find a system header, we WILL need to do some stitching.
                
                string header_name;
                if (curr_token().type == PPTOK_IDENTIFIER) {
                    //we need to replace this JUST incase
                    if (pp_replace_ident(ctx, _index) != 0) return -1;
                }

                if (curr_token().type == PPTOK_STR_LIT) {
                    //easy! its a local header
                    //we trim the header to get rid of the "", and continue on
                    header_name = string_make(curr_token().tok.raw + 1, curr_token().tok.len - 2);
                } else if (curr_token().itype == CTOK_LESS_THAN) {
                    //augh. system header.
                    //we need to start stitching.
                    //this is a quick and dirty custom string builder, and i HATE it.
                    _index++; //skip <
                    size_t old_index = _index; //for restoring later
                    
                    size_t len = 0;
                    for (; _index < ctx->tokens.len; _index++) {
                        if (curr_token().itype == CTOK_GREATER_THAN) break;
                        len += curr_token().tok.len;
                    }
                    _index = old_index;
                    if (len == 0) {
                        print_parsing_error(ctx, curr_token(), "expected header name");
                        return -1;
                    }
                    //now we know the length, we can allocate enough space for it.
                    header_name = string_alloc(len);
                    //copy in the sections of the header split up
                    size_t cursor = 0;
                    for (; _index < ctx->tokens.len; _index++) {
                        if (curr_token().itype == CTOK_GREATER_THAN) break;
                        memmove(header_name.raw + cursor, curr_token().tok.raw, curr_token().tok.len);
                        cursor += curr_token().tok.len;
                    }                   
                } else {
                    print_parsing_error(ctx, curr_token(), "expected \"header_name.h\" or <header_name.h>");
                    return -1;
                }

                //first, check if we've included this before, and if so, check pragma
                for_vec(string* file, &ctx->pragma_files) {
                    //we've done this file before, we need to toss it.
                    printf("checking if we've done "str_fmt"before\n", str_arg(*file));
                    if (string_eq(*file, header_name)) return 0x1234;
                }

                //now, we parse this header, and add it to the list of included headers, just incase pragma once happens
                //first, we create a new cobalt context
                cobalt_ctx sub_cctx = {.curr_file = header_name,
                                       .no_colour = ctx->ctx->no_colour,
                                       .include_paths = ctx->ctx->include_paths};
                printf("parsing include: "str_fmt"\n", str_arg(header_name));
                //we then pass this into parse_file
                int retval = parse_file(&sub_cctx, true);
                if (retval != 0 && retval != 0x1234 /*pragma sentinel*/) return -1;

                //look into the pctx's pragma files, and append them to this one's pragma files
                for_vec(string* file, &sub_cctx.pctx->pragma_files) {
                    vec_append(&ctx->pragma_files, *file);
                }

                size_t curr_line = curr_token().line;
                //we can then, at THIS position in the file, remove the include and related tokens, and then insert these tokens
                //we're at the end of the include, so now we just remove from hash_location to _index
                for (size_t i = 0; i < (_index - hash_location + 1); i++) {
                    vec_remove(&ctx->tokens, hash_location);
                }

                if (retval == 0x1234) continue; //skip writing due to pragma

                if (sub_cctx.pctx->tokens.len == 1) continue;

                //printf("\ninserting tokens for "str_fmt"\n", str_arg(header_name));
                size_t read_line = sub_cctx.pctx->tokens.at[0].line;
                for (size_t i = 0; i < sub_cctx.pctx->tokens.len; i++) {
                    //fix up token, since it has broken line numbers
                    token inserted_tok = sub_cctx.pctx->tokens.at[i];
                    inserted_tok.was_included = true;
                    inserted_tok.included_from = header_name;

                    if (read_line != inserted_tok.line) {
                        read_line = inserted_tok.line;
                        curr_line++;
                    }


                    //printf("inserting tok %d: "str_fmt"\n", inserted_tok.line, str_arg(inserted_tok.tok));
                    inserted_tok.line = curr_line;
                    vec_insert(&ctx->tokens, hash_location + i, inserted_tok);

                }

                continue;
            } else if (string_eq(curr_token().tok, constr("embed"))) {
                print_parsing_error(ctx, curr_token(), "TODO: embed");
                return -1;
            } else if (string_eq(curr_token().tok, constr("define"))) {
                _index+=2; //skip define and ws

                if (curr_token().type != PPTOK_IDENTIFIER) {
                    print_parsing_error(ctx, next_token(), "expected identifier");
                    return -1;
                }
                //we first need to check that the macro hasnt already been defined
                //technically, this check violates 6.10.5.2, but it shouldnt come up much.
                //FIXME?: is this valid?
                
                for_vec(macro_define* def, &ctx->defines) {
                    if (string_eq(def->name.tok, curr_token().tok)) {
                        print_parsing_error(ctx, curr_token(), "macro "str_fmt" already defined");
                        return -1;
                    }
                }
                //start new macro define
                macro_define new_def = (macro_define){.name = curr_token(),
                                                      .replacement_list = vec_new(token, 1),
                                                      .arguments = vec_new(token, 1)};



                size_t curr_line = curr_token().line;

                if (next_token().type != TOK_WHITESPACE && next_token().itype != CTOK_OPEN_PAREN) {
                    if (next_token().line != curr_token().line) continue; // for empty replacement lists
                    print_parsing_warning(ctx, next_token(), "whitespace is required after a #define directive");
                } else {
                    _index++;
                }
                if (curr_token().type == TOK_WHITESPACE) _index++;
                
                if (curr_token().itype == CTOK_OPEN_PAREN) {
                    _index++;
                    new_def.is_function = true;
                    //get argument list
                    //first, we detect
                    //"(" "..." ")"
                    skip_whitespace();
                    if (curr_token().itype == CTOK_ELLIPSIS) {
                        //add ellipsis to define, make define variadic
                        new_def.is_variadic = true;
                        vec_append(&new_def.arguments, curr_token());
                        _index++;
                        skip_whitespace();
                        if (curr_token().itype != CTOK_CLOSE_PAREN) {
                            print_parsing_error(ctx, curr_token(), "expected )");
                            return -1;
                        }
                        //we're on a close paren, so now we can fall through and just get the replacement list grab
                    }
                    //now, we detect "(" arg1, arg2, ... ")" and "(" arg1, arg2, arg3 ")" in the next one
                    if (curr_token().type == PPTOK_IDENTIFIER) {
                        //handle identifier
                        vec_append(&new_def.arguments, curr_token());
                        _index++;
                        //we've now caught the first arg, and can now blindly parse from here
                    } else if (curr_token().itype != CTOK_CLOSE_PAREN) {
                        print_parsing_error(ctx, curr_token(), "expected , or identifier");
                        return -1;
                    }

                    for (; _index < ctx->tokens.len; _index++) {
                        skip_whitespace();
                        //we look ahead for ), ",", identifiers, or ...
                        if (curr_token().itype == CTOK_CLOSE_PAREN) break;    
                        if (curr_token().itype == CTOK_COMMA) {
                            //scan ahead for identifier, ..., or )
                            if (next_token().type == TOK_WHITESPACE) _index++;

                            if (next_token().type == PPTOK_IDENTIFIER) continue;
                            if (next_token().itype == CTOK_CLOSE_PAREN) continue; 
                            if (next_token().itype == CTOK_ELLIPSIS) continue;
                            print_parsing_error(ctx, next_token(), "expected , or )");
                            return -1;
                        }
                        if (curr_token().type == PPTOK_IDENTIFIER) {
                            vec_append(&new_def.arguments, curr_token());
                            continue;
                        }
                        if (curr_token().itype == CTOK_ELLIPSIS) {
                            token new_tok = curr_token();
                            _index++;
                            skip_whitespace();
                            if (curr_token().itype != CTOK_CLOSE_PAREN) {
                                print_parsing_error(ctx, next_token(), "expected ) to end variadic function macro"); 
                                return -1;
                            }
                            new_def.is_variadic = true;
                            vec_append(&new_def.arguments, new_tok);
                            _index--;
                            continue;
                        }
                    }
                    _index++;
                    skip_whitespace();
                    //printf("curr tok after scanning #define: "str_fmt"\n", str_arg(curr_token().tok));
                }
                //we continue until the line number changes
                for (; _index < ctx->tokens.len; _index++) {
                    if (ctx->tokens.at[_index].line != curr_line) break;
                    if (ctx->tokens.at[_index].type == TOK_WHITESPACE) {
                        //we're gonna cut down the whitespace to just a single character space.
                        ctx->tokens.at[_index].tok.len = 1;
                        continue;
                    }
                    vec_append(&new_def.replacement_list, ctx->tokens.at[_index]);
                }

                _index--; //fix accidental overread
                vec_append(&ctx->defines, new_def);
                continue;
            } else if (string_eq(curr_token().tok, constr("undef"))) {
                //skip current token and the following ws
                _index+=2;
                //then, if this isnt an identifier, we know we've got a syntax error
                if (curr_token().type != PPTOK_IDENTIFIER) {
                    print_parsing_error(ctx, curr_token(), "expected identifier after #undef");
                    return -1;
                }
                //then, we search the defines list, and if we find this define, we remove it.
                //if we dont find one, thats fine.
                for_vec(macro_define* def, &ctx->defines) {
                    if (string_eq(def->name.tok, curr_token().tok)) {
                        vec_remove(&ctx->defines, _index);
                        _index--; //just incase
                        continue;
                    }
                }
                continue;
            } else if (string_eq(curr_token().tok, constr("line"))) {
                print_parsing_error(ctx, curr_token(), "TODO: line");
                return -1;
            } else if (string_eq(curr_token().tok, constr("warning"))) {
                print_parsing_error(ctx, curr_token(), "TODO: warning");
                return -1;
            } else if (string_eq(curr_token().tok, constr("error"))) {
                print_parsing_error(ctx, curr_token(), "TODO: error");
                return -1;
            } else if (string_eq(curr_token().tok, constr("pragma"))) {
                _index+=2; //skip pragma and ws
                if (string_eq(curr_token().tok, constr("once"))) {
                    //alright, we need to remove the tokens for #pragma<ws>once, and then also append it to this ctx.
                    vec_append(&ctx->pragma_files, ctx->ctx->curr_file);
                    continue;
                } else {
                    print_parsing_error(ctx, curr_token(), "unknown pragma");
                    return -1;
                }
            } else {
                print_parsing_error(ctx, curr_token(), "unknown directive "str_fmt, str_arg(curr_token().tok));
                return -1;
            }
            
        }
        if (tok->type == PPTOK_IDENTIFIER) {
            ctx->curr_macro_name = *tok;
            if (pp_replace_ident(ctx, _index) != 0) return -1;
        }
    }

    //now that we've processed all of the directives, and executed all macros, we can now delete _all_ directives
    for_vec(token* tok, &ctx->tokens) {
        if (tok->itype == CTOK_HASH && tok->after_newline == true) {
            size_t directive_start = _index;
            //we now scan until we hit a new line
            //we've filtered out all non-directives, so we can guarantee all of these lines will be directives.
            size_t directive_end = _index;
            for (; directive_end < ctx->tokens.len; directive_end++) {
                if (tok->line != ctx->tokens.at[directive_end].line) break;
            }
            //we now have the number of tokens to iterate
            printf("deleting %d tokens\n", directive_end - directive_start);    
            for (size_t i = 0; i < (directive_end - directive_start); i++) {
                vec_remove(&ctx->tokens, _index);
            }
        }
    }
    return 0;
}