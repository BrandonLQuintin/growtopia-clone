#ifndef BLOCK_TEXTURE_H
#define BLOCK_TEXTURE_H

#include <stdint.h>

#define ATLAS_SIZE 1024
#define ATLAS_COLS 32
#define TILE_TEX_SIZE 32
#define MAX_ATLAS_FRAMES 4
#define ATLAS_FRAME_MS   150
#define ATLAS_ROWS       (ATLAS_SIZE / TILE_TEX_SIZE)

void block_texture_generate(int sprite_id, int frame, unsigned char *buffer);
void block_texture_generate_atlas_frame(unsigned char *atlas_buffer, int frame_index);

#endif
