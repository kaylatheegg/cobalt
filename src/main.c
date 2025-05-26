#include "orbit.h"
#include "cobalt.h"
#include "parse/parse.h"

void parse_args(cobalt_ctx* ctx);

int main(int argc, char* argv[]) {
    cobalt_ctx ctx = {.output_path = constr(""),
                      .curr_file = constr(""),
                      .implicit_output = false,
                      .no_colour = false};

    ctx.args = vec_new(string, 1);

    for (u32 i = 0; i < argc; i++) {
        vec_append(&ctx.args, string_make(argv[i], strlen(argv[i])));
    }

    parse_args(&ctx);
    if (ctx.curr_file.len == 0) {
        printf("expected file\n");
        return -1;
    }

    if (ctx.output_path.len == 0) {
        ctx.implicit_output = true;
        ctx.output_path = strprintf(str_fmt".out", str_arg(string_make(ctx.curr_file.raw, ctx.curr_file.len - 2)));
    }

    int retval = parse_file(&ctx);
    if (retval == 0) {
        //more corpses
    }

    return 0;
}

void parse_args(cobalt_ctx* ctx) {
    int parse_state = 0; //bad. make this cleaner when more args get added
    for_vec(string* arg, &ctx->args) {
        if (parse_state == 1) {
            // blindly set this arg to output path
            ctx->output_path = *arg;
            parse_state = 0;
            continue;
        }
        if (parse_state == 0 && string_eq(*arg, constr("-o"))) {
            //we've got an output path
            //get next arg
            parse_state = 1;
            continue;
        }
        if (parse_state == 0 && string_eq(*arg, constr("-nocol"))) {
            ctx->no_colour = true;
        }
        if (parse_state == 0) {
            //scan end of this string to see if its .c
            string new_arg = *arg;
            new_arg.raw = new_arg.raw + arg->len - 2;
            new_arg.len = 2;
            if (string_eq(new_arg, constr(".c"))) {
                //we've got a file name
                ctx->curr_file = *arg;
            }
            continue;
        }
    }
}