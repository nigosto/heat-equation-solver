import sys
import math

if __name__ == "__main__":
    grid_size = int(sys.argv[1])

    cx = (grid_size - 1) / 2.0
    cy = (grid_size - 1) / 2.0
    sigma = grid_size / 6.0

    with open("central_heat_source_input.pgm", "wb") as file:
        header = f"P5\n{grid_size} {grid_size}\n255\n"
        file.write(header.encode('ascii'))

        for y in range(grid_size):
            row = bytearray(grid_size)
            for x in range(grid_size):
                dist_sq = (x - cx) ** 2 + (y - cy) ** 2
                value = 255 * math.exp(-dist_sq / (2 * sigma ** 2))
                row[x] = round(value)
            file.write(bytes(row))