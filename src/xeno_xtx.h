#ifndef XENO_XTX_H
#define XENO_XTX_H

int parse_xtx(char *filename, Texture *tex);
RGBA* apply_palettes(uint8_t *img_rgb, uint8_t *img, uint16_t w, uint16_t h, vector *mat);

#endif