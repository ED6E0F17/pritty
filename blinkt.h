#include <stdint.h>
void blinkt_colour(uint8_t led, uint8_t r, uint8_t g, uint8_t b);
void blinkt_bright(uint8_t led, uint8_t brightness);
int is_running(void);
int blinkt_setup(void);
int start_blinkt(void);
int start_rainbow(void);
void clear_blinkt(void);
void show_blinkt(void);
void show_rainbow(void);
