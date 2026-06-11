import sys

if __name__ == "__main__":
    grid_size = int(sys.argv[1])
    with open("input.pgm", "wb") as file:        
        header = f"P5\n{grid_size} {grid_size}\n255\n"
        file.write(header.encode('ascii'))

        top_row = bytes([255] * grid_size)
        file.write(top_row)

        body_row = bytes([127] + [0] * (grid_size - 2) + [127])

        for _ in range(1, grid_size):
            file.write(body_row)
