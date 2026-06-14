import sys
import math

def gaussian(dist_sq, sigma):
    return math.exp(-dist_sq / (2 * sigma ** 2))

if __name__ == "__main__":
    grid_size = int(sys.argv[1])
    sigma = grid_size / 2.5

    cx = grid_size - 1
    cy = 0

    with open("diagonal_heat_source_input.pgm", "wb") as file:
        header = f"P5\n{grid_size} {grid_size}\n255\n"
        file.write(header.encode('ascii'))

        for y in range(grid_size):
            row = bytearray(grid_size)
            for x in range(grid_size):
                dist_sq = (x - cx)**2 + (y - cy)**2
                heat = gaussian(dist_sq, sigma)
                row[x] = round(255 * heat)
            file.write(bytes(row))