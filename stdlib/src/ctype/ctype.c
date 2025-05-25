#include <ctype.h>
#include <stdio.h>

int isalnum(int c) {
    return isalpha(c) || isdigit(c);
}

int isalpha(int c) {
    return isupper(c) || islower(c);
}

int iscntrl(int c) {
    return c < 0x20 || c == 0x7f;
}

int isdigit(int c) {
    return '0' <= c && c <= '9';
}

int isgraph(int c) {
    return isalnum(c) || ispunct(c);
}

int islower(int c) {
    return 'a' <= c && c <= 'z';
}

int isprint(int c) {
    return isgraph(c) || c == ' ';
}

int ispunct(int c) {
    return ('!' <= c && c <= '/') ||
           (':' <= c && c <= '@') ||
           ('[' <= c && c <= '`') ||
           ('{' <= c && c <= '~');
}

int isspace(int c) {
    return ('\t' <= c && c <= '\r') || c == ' ';
}

int isupper(int c) {
    return 'A' <= c && c <= 'Z';
}

int isxdigit(int c) {
    return isdigit(c) || 
           ('A' <= c && c <= 'F') || 
           ('a' <= c && c <= 'f');
}

int tolower(int c) {
    if (isupper(c)) return c - 32;
    return c;
}

int toupper(int c) {
    if (islower(c)) return c + 32;
    return c;
}
