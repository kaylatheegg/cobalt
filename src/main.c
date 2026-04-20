#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>

#include "common/vec.h"
#include "common/str.h"
#include "common/util.h"
#include "cobalt.h"
#include "parse/parse.h"

void parse_args(cobalt_ctx* ctx);
void display_help();

int cobalt_main(int argc, char* argv[]);

#ifndef FUZZ
int main(int argc, char* argv[]) {
    cobalt_main(argc, argv);    
}
#else
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    //we have to do some HEAVY jank here.
    //we create a file, call cobalt_main, and then delete the file.
    FsFile* fuzz_file = fs_open("fuzz.c", true, true);
    //we do some jank here
    write(fuzz_file->handle, data, size); //LINUX SPECIFIC
    
    fs_close(fuzz_file);
    fs_destroy(fuzz_file);
    //fix permissions
    char* mode = "0666";
    chmod("fuzz.c", strtol(mode, 0, 8)); //augh.

    //then, we call into cobalt_main with the correct args
    char** argv = malloc(sizeof(*argv) * 2);
    argv[0] = "cobalt";
    argv[1] = "fuzz.c";
    int argc = 2;
    int retval = cobalt_main(argc, argv);
    return 0;  // Values other than 0 and -1 are reserved for future use.
}
#endif

int cobalt_main(int argc, char* argv[]) {
    cobalt_ctx ctx = {.output_path = strlit(""),
                      .curr_file = strlit(""),
                      .implicit_output = false,
                      .no_colour = false,
                      .include_paths = vec_new(string, 1)};
    //default family of system headers

    ctx.args = vec_new(string, 1);

    for (u32 i = 0; i < argc; i++) {
        vec_append(&ctx.args, string_make(argv[i], strlen(argv[i])));
    }

    parse_args(&ctx);
    vec_append(&ctx.include_paths, strlit("/usr/include/"));
    vec_append(&ctx.include_paths, strlit("/usr/include/linux/"));
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
    for_n(i, 0, vec_len(ctx->args)) {
        string* arg = &ctx->args[i];
        if (string_eq(*arg, strlit("-nocol"))) {
            ctx->no_colour = true;
        }

        if (string_eq(*arg, strlit("-o"))) {
            //we've got an output path
            if (i + 1 >= vec_len(ctx->args)) {
                display_help();
                exit(-1);
            }
            //get next arg
            string next_arg = ctx->args[i + 1];
            i++;
            if (next_arg.raw[0] == '-') {
                display_help();
                exit(-1);
            }
            ctx->output_path = next_arg; 
            continue;
        }

        if (string_eq(*arg, strlit("-h"))) {
            display_help();
            exit(-1);
        }


        //get first 2 chars of arg to see if its an include
        string new_arg = *arg;
        new_arg.len = 2;
        if (string_eq(new_arg, strlit("-I"))) {
            //we've got an include path!
            if (arg->len == 2) {
                if (i + 1 >= vec_len(ctx->args)) {
                    display_help();
                    exit(-1);
                }
                i++;
                vec_append(&ctx->include_paths, ctx->args[i]);
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
        if (string_eq(new_arg, strlit(".c"))) {
            //we've got a file name
            ctx->curr_file = *arg;
        }
    }
}