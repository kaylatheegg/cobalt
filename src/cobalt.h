#pragma once
#define COBALT_H

typedef struct {
    string output_path;
    string curr_file;
    bool implicit_output;
    bool no_colour;
    Vec(string) args;
} cobalt_ctx;