import socket
import pygame

WIDTH = 240
HEIGHT = 240

# Initialize Pygame and screen
pygame.init()
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption('ESP32 Stream Display')

white_color = (255, 255, 255)
screen.fill(white_color)
pygame.display.flip()

def connect_to_esp32():
    """Attempts to establish a socket connection with the ESP32."""
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(('192.168.0.26', 80))
    return s

def receive_frame(sock):
    """Receive data for one frame of pixel data."""
    frame_size = WIDTH * HEIGHT * 2  # Total bytes for one frame
    data = b''
    while len(data) < frame_size:
        packet = sock.recv(frame_size - len(data))
        if not packet:
            return None
        data += packet
    return data

# Attempt to connect to the ESP32
try:
    s = connect_to_esp32()
    print("Connection established.")
except socket.error as e:
    print(f"Failed to connect: {e}")
    pygame.quit()
    exit(1)  # Exit if initial connection failed

try:
    running = True
    while running:
        try:
            frame_data = receive_frame(s)
            if not frame_data:
                print("No frame data received, attempting to reconnect.")
                s.close()  # Close the previous connection
                s = connect_to_esp32()  # Reconnect
                continue  # Skip further processing and start receiving next frame

            # Process the entire frame
            for y in range(HEIGHT):
                for x in range(WIDTH):
                    offset = (y * WIDTH + x) * 2
                    pixel = int.from_bytes(frame_data[offset:offset + 2], byteorder='little')
                    color = ((pixel & 0xF800) >> 8, (pixel & 0x07E0) >> 3, (pixel & 0x001F) << 3)
                    screen.set_at((x, y), color)

            pygame.display.flip()  # Update the display after the full frame has been drawn
            
        except socket.error as e:
            print(f"Socket error: {e}, attempting to reconnect.")
            s.close()
            s = connect_to_esp32()

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
finally:
    s.close()
    pygame.quit()
