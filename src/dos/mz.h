#ifndef MZ_H
#define MZ_H

void LoadMzExeFromFile(char *filename);
void LoadCOMFromFile(char *filename);

void LoadMzExe(uint8_t *data, int size);
void LoadCOM(uint8_t *data, int size);

#endif //MZ_H
