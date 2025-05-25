#pragma once
#define COBALT_H

typedef struct {
    string output_path;
    string curr_file;
    bool implicit_output;
    Vec(string) args;
} cobalt_ctx;