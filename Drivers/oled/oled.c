#include "oled.h"
#include "ti_msp_dl_config.h"

#include <string.h>

#define OLED_ADDRESS (0x3CU)
#define OLED_WIDTH   (128U)
#define OLED_PAGES     (8U)

static uint8_t g_frame[OLED_WIDTH * OLED_PAGES];

static void i2cDelay(void)
{
    delay_cycles(60U);
}

static void scl(bool high)
{
    if (high) DL_GPIO_setPins(OLED_PORT, OLED_SCL_PIN);
    else DL_GPIO_clearPins(OLED_PORT, OLED_SCL_PIN);
    i2cDelay();
}

static void sda(bool high)
{
    if (high) {
        /* Release the open-drain bus; OLED modules provide the pull-up. */
        DL_GPIO_disableOutput(OLED_PORT, OLED_SDA_PIN);
    } else {
        DL_GPIO_clearPins(OLED_PORT, OLED_SDA_PIN);
        DL_GPIO_enableOutput(OLED_PORT, OLED_SDA_PIN);
    }
    i2cDelay();
}

static void start(void)
{
    sda(true);
    scl(true);
    sda(false);
    scl(false);
}

static void stop(void)
{
    sda(false);
    scl(true);
    sda(true);
}

static void writeByte(uint8_t data)
{
    uint8_t i;
    for (i = 0; i < 8U; i++) {
        sda((data & 0x80U) != 0U);
        scl(true);
        scl(false);
        data <<= 1;
    }
    /* Release SDA for ACK. The display ACK is intentionally not blocking. */
    sda(true);
    scl(true);
    scl(false);
}

static void command(uint8_t value)
{
    start();
    writeByte((uint8_t) (OLED_ADDRESS << 1));
    writeByte(0x00U);
    writeByte(value);
    stop();
}

static void glyph(char character, uint8_t columns[5])
{
    static const uint8_t digits[10][5] = {
        {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
        {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
        {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
        {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
        {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E}
    };
    static const uint8_t letterM[5] = {0x7F,0x02,0x0C,0x02,0x7F};
    static const uint8_t letterR[5] = {0x7F,0x09,0x19,0x29,0x46};
    static const uint8_t letterS[5] = {0x46,0x49,0x49,0x49,0x31};
    static const uint8_t letterT[5] = {0x01,0x01,0x7F,0x01,0x01};
    static const uint8_t letterA[5] = {0x7E,0x11,0x11,0x11,0x7E};
    static const uint8_t letterB[5] = {0x7F,0x49,0x49,0x49,0x36};
    static const uint8_t letterC[5] = {0x3E,0x41,0x41,0x41,0x22};
    static const uint8_t letterD[5] = {0x7F,0x41,0x41,0x22,0x1C};
    static const uint8_t letterE[5] = {0x7F,0x49,0x49,0x49,0x41};
    static const uint8_t letterH[5] = {0x7F,0x08,0x08,0x08,0x7F};
    static const uint8_t letterI[5] = {0x00,0x41,0x7F,0x41,0x00};
    static const uint8_t letterL[5] = {0x7F,0x40,0x40,0x40,0x40};
    static const uint8_t letterN[5] = {0x7F,0x02,0x04,0x08,0x7F};
    static const uint8_t letterO[5] = {0x3E,0x41,0x41,0x41,0x3E};
    static const uint8_t letterP[5] = {0x7F,0x09,0x09,0x09,0x06};
    static const uint8_t letterV[5] = {0x1F,0x20,0x40,0x20,0x1F};
    static const uint8_t letterG[5] = {0x3E,0x41,0x49,0x49,0x7A};
    static const uint8_t letterU[5] = {0x3F,0x40,0x40,0x40,0x3F};
    static const uint8_t letterK[5] = {0x7F,0x08,0x14,0x22,0x41};
    static const uint8_t dot[5] = {0x00,0x60,0x60,0x00,0x00};
    const uint8_t *source = NULL;

    if ((character >= '0') && (character <= '9')) {
        source = digits[(uint8_t) character - '0'];
    } else if (character == 'M') source = letterM;
    else if (character == 'R') source = letterR;
    else if (character == 'S') source = letterS;
    else if (character == 'T') source = letterT;
    else if (character == 'A') source = letterA;
    else if (character == 'B') source = letterB;
    else if (character == 'C') source = letterC;
    else if (character == 'D') source = letterD;
    else if (character == 'E') source = letterE;
    else if (character == 'H') source = letterH;
    else if (character == 'I') source = letterI;
    else if (character == 'L') source = letterL;
    else if (character == 'N') source = letterN;
    else if (character == 'O') source = letterO;
    else if (character == 'P') source = letterP;
    else if (character == 'V') source = letterV;
    else if (character == 'G') source = letterG;
    else if (character == 'U') source = letterU;
    else if (character == 'K') source = letterK;
    else if (character == '.') source = dot;
    if (source != NULL) memcpy(columns, source, 5U);
    else memset(columns, 0, 5U);
}

static void drawChar(uint8_t x, uint8_t page, char character)
{
    uint8_t columns[5];
    uint8_t i;
    glyph(character, columns);
    for (i = 0; i < 5U; i++) {
        g_frame[((uint16_t) page * OLED_WIDTH) + x + i] = columns[i];
    }
}

static void drawText(uint8_t x, uint8_t page, const char *text)
{
    while ((*text != '\0') && (x <= (OLED_WIDTH - 5U))) {
        drawChar(x, page, *text++);
        x += 6U;
    }
}

void OLED_Init(void)
{
    static const uint8_t initSequence[] = {
        0xAE,0x20,0x00,0xB0,0xC8,0x00,0x10,0x40,0x81,0x7F,
        0xA1,0xA6,0xA8,0x3F,0xA4,0xD3,0x00,0xD5,0x80,0xD9,
        0xF1,0xDA,0x12,0xDB,0x40,0x8D,0x14,0xAF
    };
    uint8_t i;
    delay_cycles(CPUCLK_FREQ / 20U);
    for (i = 0; i < sizeof(initSequence); i++) command(initSequence[i]);
    OLED_Clear();
    OLED_Update();
}

void OLED_Clear(void)
{
    memset(g_frame, 0, sizeof(g_frame));
}

void OLED_SetPixel(uint8_t x, uint8_t y, bool enabled)
{
    uint16_t index;
    uint8_t mask;
    if ((x >= OLED_WIDTH) || (y >= 64U)) return;
    index = ((uint16_t) (y >> 3) * OLED_WIDTH) + x;
    mask = (uint8_t) (1U << (y & 7U));
    if (enabled) g_frame[index] |= mask;
    else g_frame[index] &= (uint8_t) ~mask;
}

void OLED_Update(void)
{
    uint8_t page;
    uint8_t x;
    for (page = 0; page < OLED_PAGES; page++) {
        command((uint8_t) (0xB0U + page));
        command(0x00U);
        command(0x10U);
        start();
        writeByte((uint8_t) (OLED_ADDRESS << 1));
        writeByte(0x40U);
        for (x = 0; x < OLED_WIDTH; x++) {
            writeByte(g_frame[((uint16_t) page * OLED_WIDTH) + x]);
        }
        stop();
    }
}

static void updatePage(uint8_t page)
{
    uint8_t x;
    command((uint8_t) (0xB0U + page));
    command(0x00U);
    command(0x10U);
    start();
    writeByte((uint8_t) (OLED_ADDRESS << 1));
    writeByte(0x40U);
    for (x = 0; x < OLED_WIDTH; x++) {
        writeByte(g_frame[((uint16_t) page * OLED_WIDTH) + x]);
    }
    stop();
}

void OLED_ShowStatus(uint8_t mode, uint32_t elapsedMs, bool running)
{
    uint32_t tenths = elapsedMs / 100U;
    uint8_t x;

    OLED_Clear();
    drawChar(0U, 0U, 'M');
    drawChar(7U, 0U, (char) ('0' + mode));
    drawChar(0U, 2U, running ? 'R' : 'S');
    drawChar(7U, 2U, 'T');

    x = 20U;
    if (tenths >= 1000U) drawChar(x, 2U, (char) ('0' + ((tenths / 1000U) % 10U)));
    x += 6U;
    if (tenths >= 100U) drawChar(x, 2U, (char) ('0' + ((tenths / 100U) % 10U)));
    x += 6U;
    drawChar(x, 2U, (char) ('0' + ((tenths / 10U) % 10U)));
    x += 6U;
    drawChar(x, 2U, (char) ('0' + (tenths % 10U)));
    updatePage(0U);
    updatePage(2U);
}

void OLED_ShowMenu(uint8_t mode, uint32_t elapsedMs, bool running)
{
    static const char *modeNames[6] = {
        "TRACK TEST", "LAP STOP", "BALL TEST",
        "AB BALANCE", "LAP CENTER", "LAP TARGET"
    };
    static uint8_t previousMode;
    static bool previousRunning;
    uint32_t seconds = elapsedMs / 1000U;
    uint8_t tenth = (uint8_t) ((elapsedMs / 100U) % 10U);
    uint8_t x = 30U;

    if ((mode < 1U) || (mode > 6U)) mode = 1U;
    if ((mode != previousMode) || (running != previousRunning)) {
        memset(&g_frame[0U * OLED_WIDTH], 0, OLED_WIDTH);
        memset(&g_frame[2U * OLED_WIDTH], 0, OLED_WIDTH);
        memset(&g_frame[4U * OLED_WIDTH], 0, OLED_WIDTH);
        memset(&g_frame[5U * OLED_WIDTH], 0, OLED_WIDTH);
        drawText(0U, 0U, "MODE");
        drawChar(30U, 0U, (char) ('0' + mode));
        drawText(0U, 2U, modeNames[mode - 1U]);
        drawText(0U, 4U, running ? "RUN" : "HOLD 1.5S");
        updatePage(0U);
        updatePage(2U);
        updatePage(4U);
        updatePage(5U);
        previousMode = mode;
        previousRunning = running;
    }

    memset(&g_frame[6U * OLED_WIDTH], 0, OLED_WIDTH);
    if (seconds > 999U) seconds = 999U;
    if (seconds >= 100U) {
        drawChar(x, 6U, (char) ('0' + ((seconds / 100U) % 10U)));
        x += 6U;
    }
    if (seconds >= 10U) {
        drawChar(x, 6U, (char) ('0' + ((seconds / 10U) % 10U)));
        x += 6U;
    }
    drawChar(x, 6U, (char) ('0' + (seconds % 10U)));
    x += 6U;
    drawChar(x, 6U, '.');
    x += 6U;
    drawChar(x, 6U, (char) ('0' + tenth));
    drawText((uint8_t) (x + 6U), 6U, "S");

    updatePage(6U);
}

void OLED_ShowTrackPattern(
    uint8_t rawHighPattern, uint8_t pattern, bool lineVisible)
{
    uint8_t i;

    memset(&g_frame[4U * OLED_WIDTH], 0, OLED_WIDTH);
    memset(&g_frame[5U * OLED_WIDTH], 0, OLED_WIDTH);
    if (!lineVisible) {
        drawText(0U, 4U, "LOST");
    } else {
        drawText(0U, 4U, "B");
        for (i = 0; i < 5U; i++) {
            drawChar((uint8_t) (12U + (i * 7U)), 4U,
                (pattern & (1U << i)) ? '1' : '0');
        }
    }
    drawText(0U, 5U, "R");
    for (i = 0; i < 5U; i++) {
        drawChar((uint8_t) (12U + (i * 7U)), 5U,
            (rawHighPattern & (1U << i)) ? '1' : '0');
    }
    updatePage(4U);
    updatePage(5U);
}

void OLED_ShowImuStatus(bool connected, bool calibrated)
{
    memset(&g_frame[7U * OLED_WIDTH], 0, OLED_WIDTH);
    if (!connected) {
        drawText(0U, 7U, "IMU ERR");
    } else if (!calibrated) {
        drawText(0U, 7U, "IMU CAL");
    } else {
        drawText(0U, 7U, "IMU OK");
    }
    updatePage(7U);
}
