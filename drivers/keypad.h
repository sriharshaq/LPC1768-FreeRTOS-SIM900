
#ifndef __keypad_h__
#define __keypad_h__

// Port (P1)
#define KEY_C0 19
#define KEY_C1 20
#define KEY_C2 21
#define KEY_C3 22
#define KEY_R0 23
#define KEY_R1 24
#define KEY_R2 25
#define KEY_R3 -26

#define NO_KEY_PRESS 0

extern const uint8_t key_map[];

extern uint8_t read_keypad(void);

#endif