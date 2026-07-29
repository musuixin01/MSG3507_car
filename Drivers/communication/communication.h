#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void Bluetooth_Send(const uint8_t *data, size_t length);
bool Bluetooth_ReadByte(uint8_t *data);
void K210_Send(const uint8_t *data, size_t length);
bool K210_ReadByte(uint8_t *data);

#endif
