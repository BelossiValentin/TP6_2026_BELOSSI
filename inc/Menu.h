#ifndef MENU_H_
#define MENU_H_

#include <stdint.h>

void Menu_init(void);
void Menu_FSM(char tecla, uint8_t backlight_encendido);

#endif /* MENU_H_ */

