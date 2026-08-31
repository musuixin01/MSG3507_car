#include "communication.h"
#include "ti_msp_dl_config.h"

static void sendBytes(UART_Regs *uart, const uint8_t *data, size_t length)
{
    size_t i;
    if (data == NULL) return;
    for (i = 0; i < length; i++) {
        DL_UART_Main_transmitDataBlocking(uart, data[i]);
    }
}

static bool readByte(UART_Regs *uart, uint8_t *data)
{
    if ((data == NULL) || DL_UART_Main_isRXFIFOEmpty(uart)) {
        return false;
    }
    *data = DL_UART_Main_receiveData(uart);
    return true;
}

void K210_Send(const uint8_t *data, size_t length)
{
    sendBytes(K210_INST, data, length);
}

bool K210_ReadByte(uint8_t *data)
{
    return readByte(K210_INST, data);
}
