#ifndef MOUSE_H
#define MOUSE_H

void MouseInterrupt();
void MouseMotion(int x, int y);
void MouseButtonDown(uint8_t button);
void MouseButtonUp(uint8_t button);

#endif
