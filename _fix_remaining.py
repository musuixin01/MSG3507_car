import sys

# === Fix oled.c ===
with open(r"D:\E_Lab_Base\02_Projects\Keil\MSP3507\MSG3507_car\Drivers\oled\oled.c", "r", encoding="utf-8") as f:
    c = f.read()

# Remove ADC comment lines
for pattern in [
    "/* Debug externs for ADC tracking raw values (defined in line_follow.c) */\n",
    "/* Debug access to ADC tracking raw values (declared in line_follow.h) */\n",
]:
    c = c.replace(pattern, "")

# Remove any remaining blank lines from the top
while c.startswith("\n"):
    c = c[1:]

# Find and replace OLED_ShowTrackPattern
idx = c.find("void OLED_ShowTrackPattern")
brace_idx = c.index("{", idx)
bc = 0
i = brace_idx
while i < len(c):
    if c[i] == "{": bc += 1
    elif c[i] == "}": bc -= 1
    if bc == 0: break
    i += 1
old_func = c[idx:i+1]

new_func = "void OLED_ShowTrackPattern(\n    uint8_t rawHighPattern, uint8_t pattern, bool lineVisible)\n{\n    uint8_t i;\n\n    memset(&g_frame[4U * OLED_WIDTH], 0, OLED_WIDTH);\n    memset(&g_frame[5U * OLED_WIDTH], 0, OLED_WIDTH);\n    if (!lineVisible) {\n        drawText(0U, 4U, \"LOST\");\n        updatePage(4U);\n        updatePage(5U);\n        return;\n    } else {\n        drawText(0U, 4U, \"B\");\n        for (i = 0; i < 5U; i++) {\n            drawChar((uint8_t)(12U + (i * 7U)), 4U,\n                (pattern & (1U << i)) ? \'1\' : \'0\');\n        }\n    }\n    drawText(0U, 5U, \"R\");\n    for (i = 0; i < 5U; i++) {\n        drawChar((uint8_t)(12U + (i * 7U)), 5U,\n            (rawHighPattern & (1U << i)) ? \'1\' : \'0\');\n    }\n    updatePage(4U);\n    updatePage(5U);\n}\n"

c = c.replace(old_func, new_func)

with open(r"D:\E_Lab_Base\02_Projects\Keil\MSP3507\MSG3507_car\Drivers\oled\oled.c", "w", encoding="utf-8") as f:
    f.write(c)

print("oled.c ADC refs:", c.count("g_trackAdcRaw"), c.count("g_trackAdcValid"))

# === Fix line_follow.h ===
with open(r"D:\E_Lab_Base\02_Projects\Keil\MSP3507\MSG3507_car\App\line_follow.h", "r", encoding="utf-8") as f:
    h = f.read()

for pattern in [
    "extern volatile uint16_t g_trackAdcRaw[6];\n",
    "extern volatile uint16_t g_trackAdcWhiteReferenceDebug;\n",
    "extern volatile uint16_t g_trackAdcBlackReferenceDebug;\n",
    "extern volatile uint16_t g_trackAdcThresholdDebug;\n",
    "extern volatile uint8_t g_trackAdcValidDebug;\n",
]:
    h = h.replace(pattern, "")

with open(r"D:\E_Lab_Base\02_Projects\Keil\MSP3507\MSG3507_car\App\line_follow.h", "w", encoding="utf-8") as f:
    f.write(h)

print("line_follow.h ADC refs:", h.count("g_trackAdcRaw"), h.count("g_trackAdcWhite"), h.count("g_trackAdcValid"))

print("All fixes applied!")
