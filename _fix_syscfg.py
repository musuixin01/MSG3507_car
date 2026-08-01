import re

with open(r'D:\E_Lab_Base\02_Projects\Keil\MSP3507\MSG3507_car\App\line_follow.c', 'r', encoding='utf-8') as f:
    content = f.read()

with open(r'D:\E_Lab_Base\02_Projects\Keil\MSP3507\MSG3507_car\App\line_follow.h', 'r', encoding='utf-8') as f:
    header = f.read()

print('=== line_follow.c stats ===')
print('Length:', len(content))

# Find remaining ADC refs
import re
adc_patterns = ['g_trackWhiteReference', 'g_trackBlackReference', 'g_trackContrast', 
                'g_trackAdcRaw', 'g_trackAdcValidDebug', 'g_trackAdcWhite',
                'g_trackAdcBlack', 'g_trackAdcThreshold', 'TRACK_ADC',
                'TRACK_SENSOR_PIN_ALL', 'readTrackAdc', 'waitAdcResult']
for pat in adc_patterns:
    count = content.count(pat)
    if count > 0:
        print(f'  {pat}: {count} occurrences')

# Find current CalibrateSensors function
idx = content.find('void LineFollow_CalibrateSensors')
if idx >= 0:
    start_brace = content.index('{', idx)
    brace_count = 0
    i = start_brace
    while i < len(content):
        if content[i] == '{': brace_count += 1
        elif content[i] == '}':
            brace_count -= 1
            if brace_count == 0:
                break
        i += 1
    print(f'\n=== CalibrateSensors (lines {content[:idx].count(chr(10))+1} to {content[:i+1].count(chr(10))+1}) ===')
    print(content[idx:i+1])
