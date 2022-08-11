#ifndef XENO_LEX_H
#define XENO_LEX_H

#include <stdint.h>
#include "xenotool.h"
int64_t parse_lex(char *filename, Model *model, Texture *tex);
Material parse_materialraw(MaterialRaw mr, Texture *tex);
#endif