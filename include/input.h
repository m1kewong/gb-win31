#ifndef GBW_INPUT_H
#define GBW_INPUT_H

#include <gb/gb.h>

typedef struct InputState {
    UINT8 held;
    UINT8 pressed;
    UINT8 released;
    UINT8 repeated;
} InputState;

void input_init(void);
void input_update(void);
const InputState *input_get(void);

#endif
