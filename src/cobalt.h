#pragma once
#define COBALT_H

#include "common/str.h"
#include "common/vec.h"

typedef struct _parser_ctx parser_ctx;

typedef struct {
    string output_path;
    string curr_file;
    bool implicit_output;
    bool no_colour;
    Vec(string) args;
    parser_ctx* pctx;
    Vec(string) include_paths;
} cobalt_ctx;