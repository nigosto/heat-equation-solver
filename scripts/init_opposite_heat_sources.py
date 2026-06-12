import sys
import math

def gaussian(dist_sq, sigma):
    return math.exp(-dist_sq / (2 * sigma ** 2))

if __name__ == "__main__":
    grid_size = int(sys.argv[1])
    sigma = grid_size / 5.0

    cx = (grid_size - 1) / 2.0
    top_source    = (cx, 0)
    bottom_source = (cx, grid_size - 1)

    with open("opposite_heat_sources_input.pgm", "wb") as file:
        header = f"P5\n{grid_size} {grid_size}\n255\n"
        file.write(header.encode('ascii'))

        for y in range(grid_size):
            row = bytearray(grid_size)
            for x in range(grid_size):
                h_top    = gaussian((x - top_source[0])**2    + (y - top_source[1])**2,    sigma)
                h_bottom = gaussian((x - bottom_source[0])**2 + (y - bottom_source[1])**2, sigma)

                heat = min(1.0, h_top + h_bottom)
                row[x] = round(255 * heat)
            file.write(bytes(row))