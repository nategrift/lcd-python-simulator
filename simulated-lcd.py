import pygame
import sys
import serial
from collections import deque

SCREEN_WIDTH = 240
SCREEN_HEIGHT = 240

BLACK = (0, 0, 0)


def main():
    pygame.init()
    screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
    pygame.display.set_caption("Screen Simulator")
    screen.fill(BLACK)
    serial_port = serial.Serial('/dev/tty.usbserial-1420', 115200)

    running = True
    x = 0
    y = 0

    header = bytearray([0xAB, 0xCD, 0x12, 0x34])
    buffer = deque()

    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
        while serial_port.in_waiting >= 1:
            data = serial_port.read(1)
            buffer.extend(data)

            if len(buffer) >= 4 and bytearray(buffer)[-4:] == header:
                buffer.clear()
                print('Header found, reading more data...')
                x = 0
                y = 0
                screen.fill(BLACK)
                break

        if len(buffer) >= 2 * SCREEN_WIDTH:
            print('Rendering row: ' + str(y))
            for _ in range(SCREEN_WIDTH):
                if len(buffer) >= 2:
                    hi_byte = buffer.popleft()
                    lo_byte = buffer.popleft()
                    color_value = (hi_byte << 8) | lo_byte
                    red = ((color_value >> 11) & 0x1F) * 255 // 31
                    green = ((color_value >> 5) & 0x3F) * 255 // 63
                    blue = (color_value & 0x1F) * 255 // 31

                    # Set the pixel from the content we have
                    screen.set_at((x, y), (red, green, blue))
                    x += 1
                    if x >= SCREEN_WIDTH:
                        x = 0
                        y += 1
                        buffer.clear()
                        if y >= SCREEN_HEIGHT:
                            y = 0
        pygame.display.flip()

    serial_port.close()
    pygame.quit()
    sys.exit()

if __name__ == "__main__":
    main()
