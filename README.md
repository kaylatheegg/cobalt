# Cobalt C Compiler
cobalt is a (hopefully) C23 conformant C compiler, powered by the [Iron Target Independent Code Generator](https://github.com/spsandwichman/coyote) (currently part of the coyote compiler).
Currently, it is up to translation phase 3, with the start of work on phase 4.

## Building
### Prerequisites
You need a C compiler that supports GNU extensions (like gcc) and C23 (so no TCC! sorry...). 

### Building and running
To build cobalt, first clone it, and then in the root of the repo, type
```shell
make clean
```
This creates the dependency file structure (this is a MASSIVE hack, and if you have a better solution please open an issue or PR!)

Then, to build, you can either use 
```shell 
make debug 
``` 
OR 
```shell 
make build
``` 
Debug disables optimisations, enables debug info, and links a performance analyser.

To run it, type `./cobalt filename`. Currently, the documentation is a little... sparse, so if you have any questions feel free to open an issue or contact me.

## Known issues
- Do not attempt to build with files that cyclically include eachother with include guards, as cobalt does not yet support #ifdef.