import re

fpath = r'D:\E_Lab_Base\02_Projects\Keil\MSP3507\MSG3507_car\App\line_follow.c'

with open(fpath, 'r', encoding='utf-8') as f:
    content = f.read()

# === Helper to find full function body ===
def find_func_body(text, start_marker):
    idx = text.find(start_marker)
    if idx < 0:
        return -1, -1, ''
    start_brace = text.index('{', idx)
    brace_count = 0
    i = start_brace
    while i < len(text):
        if text[i] == '{':
            brace_count += 1
        elif text[i] == '}':
            brace_count -= 1
            if brace_count == 0:
                return idx, i + 1, text[idx:i+1]
        i += 1
    return -1, -1, ''

# === Replace CalibrateSensors ===
_, _, old_cal = find_func_body(content, 'void LineFollow_CalibrateSensors(void)')

new_cal = '''void LineFollow_CalibrateSensors(void)
{
    uint8_t sensorValues[TRACK_SENSOR_COUNT];
    uint8_t sample;
    uint8_t index;
    uint8_t validSamples = 0U;

    stopMotorOutput();
    for (sample = 0U; sample < 20U; sample++) {
        readTrackSensors(sensorValues);
        validSamples++;
    }

    if (validSamples != 0U) {
        for (index = 0U; index < TRACK_SENSOR_COUNT; index++) {
            g_trackFilteredRaw[index] = 0.0f;
            g_trackActive[index] = false;
        }
        g_trackFilterInitialized = true;
    } else {
        g_trackFilterInitialized = false;
    }

    g_runState = LINE_STATE_WAIT_START;
    clearControllerState();
    WheelSpeedPI_Reset();
    updateDebugState();
}
'''

content = content.replace(old_cal, new_cal)

# === Replace LineFollow_Read ===
_, _, old_read = find_func_body(content, 'LineObservation LineFollow_Read(void)')

new_read = '''LineObservation LineFollow_Read(void)
{
    LineObservation result = {0};
    float weightedSum = 0.0f;
    float activeSum = 0.0f;
    float instantaneousError = 0.0f;
    uint8_t sensorValues[TRACK_SENSOR_COUNT];
    uint8_t activeCount = 0U;
    uint8_t index;

    readTrackSensors(sensorValues);

    g_lastRawHighPattern = 0U;
    for (index = 0U; index < TRACK_SENSOR_COUNT; index++) {
        /* sensorValues: 1 = white (off line), 0 = black (on line) */
        bool onBlackLine = (sensorValues[index] == 0U);

        if (onBlackLine) {
            result.pattern |= (uint8_t)(1U << index);
            activeCount++;
            weightedSum += g_trackPosition[index];
            activeSum += 1.0f;
        } else {
            g_lastRawHighPattern |= (uint8_t)(1U << index);
        }
        g_trackActive[index] = onBlackLine;
    }

    /* Compute error: weighted average of active sensor positions */
    if (activeCount > 0U) {
        instantaneousError = weightedSum / activeSum;
    } else if (g_lastLineVisible) {
        instantaneousError = g_lastVisibleError;
    } else {
        instantaneousError = 0.0f;
    }
    result.error = instantaneousError;

    /* Line visible if at least one sensor sees black */
    if (activeCount >= LINE_LOST_ACTIVE_COUNT) {
        result.lineVisible = true;
        g_lostSamples = 0U;
    } else if (activeCount == 0U) {
        if (g_lastLineVisible) {
            if (g_lostSamples < LINE_LOST_CONFIRM_SAMPLES) {
                g_lostSamples++;
                result.lineVisible = true;
            } else {
                result.lineVisible = false;
            }
        } else {
            result.lineVisible = false;
        }
    } else {
        /* Between 0 and LINE_LOST_ACTIVE_COUNT - partial line */
        result.lineVisible = true;
        g_lostSamples = 0U;
    }

    /* Marker detection (all sensors on line = thick marker) */
    if (activeCount >= TRACK_SENSOR_COUNT) {
        if (g_markerDetectSamples < A_MARKER_DETECT_SAMPLES) {
            g_markerDetectSamples++;
        }
    } else {
        g_markerDetectSamples = 0U;
    }

    /* Marker approach detection */
    if (activeCount <= A_MARKER_APPROACH_MAX_ACTIVE &&
        (activeCount > 0U) &&
        result.lineVisible &&
        (activeSum > 0.0f)) {
        if (g_markerApproachSamples < A_MARKER_APPROACH_SAMPLES) {
            g_markerApproachSamples++;
        }
    } else {
        g_markerApproachSamples = 0U;
    }

    result.marker = (g_markerDetectSamples >= A_MARKER_DETECT_SAMPLES);
    g_lineActiveCountDebug = activeCount;
    g_lineMarkerDebug = result.marker ? 1U : 0U;
    g_lineMarkerApproachSamplesDebug = g_markerApproachSamples;

    if (result.lineVisible && !result.marker) {
        g_lastVisibleError = result.error;
    }

    g_lastPattern = result.pattern;
    g_lastLineVisible = result.lineVisible;
    return result;
}
'''

content = content.replace(old_read, new_read)

# === Remove all ADC-related static variables, externs, and debug vars ===
# Remove the ADC-related declarations
adc_lines = [
    'static float g_trackWhiteReference;',
    'static float g_trackBlackReference;',
    'static float g_trackContrast;',
    'static bool g_trackFilterInitialized;',
    'extern volatile uint16_t g_trackAdcRaw[6];',
    'extern volatile uint16_t g_trackAdcWhiteReferenceDebug;',
    'extern volatile uint16_t g_trackAdcBlackReferenceDebug;',
    'extern volatile uint16_t g_trackAdcThresholdDebug;',
    'extern volatile uint8_t g_trackAdcValidDebug;',
]

for adc_line in adc_lines:
    # Remove line + trailing newline
    content = content.replace(adc_line + '\\n', '')

# Remove volatile ADC debug vars from variables section
# g_trackAdcRaw, etc.
content = content.replace(
    'volatile uint16_t g_trackAdcRaw[TRACK_SENSOR_COUNT];\\n', '')
content = content.replace(
    'volatile uint16_t g_trackAdcWhiteReferenceDebug;\\n', '')
content = content.replace(
    'volatile uint16_t g_trackAdcBlackReferenceDebug;\\n', '')
content = content.replace(
    'volatile uint16_t g_trackAdcThresholdDebug;\\n', '')
content = content.replace(
    'volatile uint8_t g_trackAdcValidDebug;\\n', '')

# Remove tracking init in LineFollow_Init - keep clearControllerState and calibrate
# The old init had ADC-related stuff we should clean up
old_init, _, old_init_body = find_func_body(content, 'void LineFollow_Init(void)')
if old_init >= 0:
    # Just keep the init function as-is since it already just calls clear and calibrate
    pass

with open(fpath, 'w', encoding='utf-8') as f:
    f.write(content)

print('Done. Remaining old ADC refs:')
for pat in ['g_trackWhiteReference', 'g_trackBlackReference', 'g_trackContrast',
            'g_trackAdcRaw', 'g_trackAdcValid', 'g_trackAdcWhite', 'g_trackAdcBlack',
            'g_trackAdcThreshold', 'TRACK_ADC', 'waitAdcResult', 'readTrackAdc',
            'blackStrength', 'normalizeStrength']:
    cnt = content.count(pat)
    if cnt > 0:
        print(f'  WARN: {pat} still has {cnt} occurrence(s)')
print('Check complete.')
