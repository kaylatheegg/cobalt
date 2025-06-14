#define xyz

#ifdef xyz
    meow
#endif

#ifndef xyz
    meow2
#endif

#undef xyz

#ifdef xyz
    meow ERROR
#endif

#ifdef xyz
    meow ERROR2
#elifndef xyz
    meow4
#endif