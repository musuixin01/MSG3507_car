# MaixPy/OpenMV-compatible reference program for the K210 vision module.
# UART protocol to MSPM0: X:+123\n (millimetres from beam centre).
import sensor
import image
import time
from machine import UART
from fpioa_manager import fm

fm.register(6, fm.fpioa.UART1_TX, force=True)
fm.register(7, fm.fpioa.UART1_RX, force=True)
uart = UART(UART.UART1, 115200, 8, 0, 1, timeout=20, read_buf_len=64)

sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.skip_frames(time=1500)
sensor.set_auto_gain(False)
sensor.set_auto_whitebal(False)

# Calibrate these from the installed top camera image.
BEAM_X_MIN = 35
BEAM_X_MAX = 285
BEAM_Y_MIN = 85
BEAM_HEIGHT = 90
BEAM_LENGTH_MM = 250

clock = time.clock()
while True:
    clock.tick()
    img = sensor.snapshot()
    roi = (BEAM_X_MIN, BEAM_Y_MIN, BEAM_X_MAX - BEAM_X_MIN, BEAM_HEIGHT)
    circles = img.find_circles(
        roi=roi, threshold=2800, x_margin=20, y_margin=15,
        r_margin=15, r_min=5, r_max=18, r_step=2)

    if circles:
        # Prefer the strongest/largest detected circle.
        ball = max(circles, key=lambda c: c.magnitude())
        x_mm = int(
            ((ball.x() - BEAM_X_MIN) * BEAM_LENGTH_MM /
             (BEAM_X_MAX - BEAM_X_MIN)) - (BEAM_LENGTH_MM / 2))
        if x_mm < -125:
            x_mm = -125
        if x_mm > 125:
            x_mm = 125
        uart.write("X:%+d\n" % x_mm)
        img.draw_circle(ball.x(), ball.y(), ball.r(), color=(255, 0, 0))
        img.draw_cross(ball.x(), ball.y(), color=(0, 255, 0))
    img.draw_rectangle(roi, color=(0, 0, 255))
