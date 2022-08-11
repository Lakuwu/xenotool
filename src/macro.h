#ifndef MACRO_H
#define MACRO_H

#define CLAMP(a, min, max) ( MIN( MAX((a), (min)), (max)) )
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#endif