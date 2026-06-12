import sys
import math

if __name__ == "__main__":
    grid_size = int(sys.argv[1])
    sigma = grid_size / 10.0

    with open("border_heat_sources_input.pgm", "wb") as file:
        header = f"P5\n{grid_size} {grid_size}\n255\n"
        file.write(header.encode('ascii'))

        for y in range(grid_size):
            row = bytearray(grid_size)
            for x in range(grid_size):
                d_top   = y
                d_left  = x
                d_right = (grid_size - 1) - x

                h_top   = 1.0 * math.exp(-(d_top   ** 2) / (2 * sigma ** 2))
                h_left  = 0.5 * math.exp(-(d_left  ** 2) / (2 * sigma ** 2))
                h_right = 0.5 * math.exp(-(d_right ** 2) / (2 * sigma ** 2))

                heat = min(1.0, h_top + h_left + h_right)

                row[x] = round(255 * heat)
            file.write(bytes(row))