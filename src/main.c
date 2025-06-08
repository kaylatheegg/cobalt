#include "orbit.h"
#include "cobalt.h"
#include "parse/parse.h"

void parse_args(cobalt_ctx* ctx);
void display_help();


int main(int argc, char* argv[]) {
    cobalt_ctx ctx = {.output_path = constr(""),
                      .curr_file = constr(""),
                      .implicit_output = false,
                      .no_colour = false,
                      .include_paths = vec_new(string, 1)};
    //default family of system headers

    ctx.args = vec_new(string, 1);

    for (u32 i = 0; i < argc; i++) {
        vec_append(&ctx.args, string_make(argv[i], strlen(argv[i])));
    }

    parse_args(&ctx);
    vec_append(&ctx.include_paths, constr("/usr/include/"));
    vec_append(&ctx.include_paths, constr("/usr/include/linux/"));
    if (ctx.curr_file.len == 0) {
        display_help();
        return -1;
    }

    if (ctx.output_path.len == 0) {
        ctx.implicit_output = true;
        ctx.output_path = strprintf(str_fmt".out", str_arg(string_make(ctx.curr_file.raw, ctx.curr_file.len - 2)));
    }

    int retval = parse_file(&ctx, false);
    if (retval == 0) {
        //more corpses
    }

    return 0;
}

void display_help() {
    printf("Usage: ./cobalt filename.c -o filename\n");
    printf("Cobalt C Compiler options:\n");
    printf("\t -o <filename>:     Specify an output filename\n");
    printf("\t -I <path>:         Specify an include path that is searched before the system defaults\n");
    printf("\t -nocol:            Disables ansi escape sequences during printing\n");
    printf("\t -h:                Prints this help info\n");
    return;
}

void parse_args(cobalt_ctx* ctx) {
    for_vec(string* arg, &ctx->args) {
        if (string_eq(*arg, constr("-nocol"))) {
            ctx->no_colour = true;
        }

        if (string_eq(*arg, constr("-o"))) {
            //we've got an output path
            if (_index + 1 >= ctx->args.len) {
                display_help();
                exit(-1);
            }
            //get next arg
            string arg = ctx->args.at[_index + 1];
            _index++;
            if (arg.raw[0] == '-') {
                display_help();
                exit(-1);
            }
            ctx->output_path =arg; 
            continue;
        }        

        if (string_eq(*arg, constr("-h"))) {
            display_help();
            exit(-1);
        }


        //get first 2 chars of arg to see if its an include
        string new_arg = *arg;
        new_arg.len = 2;
        if (string_eq(new_arg, constr("-I"))) {
            //we've got an include path!
            if (arg->len == 2) {
                //we do something... sinister
                if (_index + 1 >= ctx->args.len) {
                    display_help();
                    exit(-1);
                }
                _index++;
                vec_append(&ctx->include_paths, ctx->args.at[_index]);
                continue;                
            }

            string path = string_make(arg->raw + 2, arg->len - 2);
            vec_append(&ctx->include_paths, path);
            continue;
        }

        //scan end of this string to see if its .c
        new_arg = *arg;
        new_arg.raw = new_arg.raw + arg->len - 2;
        new_arg.len = 2;
        if (string_eq(new_arg, constr(".c"))) {
            //we've got a file name
            ctx->curr_file = *arg;
        }
    }
}