#ifndef BLOCK_TEXTURE_H
#define BLOCK_TEXTURE_H

#include <stdint.h>

#define ATLAS_SIZE 512
#define ATLAS_COLS 16
#define TILE_TEX_SIZE 32

void block_texture_init(void);
void block_texture_generate(int sprite_id, unsigned char *buffer);
void block_texture_generate_atlas(unsigned char *atlas_buffer);

#endif
