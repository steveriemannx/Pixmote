#!/usr/bin/env python3
"""Convert a binary PPM (P6) screenshot to PNG using only the standard library.

Usage: python3 tools/ppm2png.py [--scale N] input.ppm output.png

The pixmote demo writes PPM files so it needs no image library; this helper turns
them into PNGs for documentation. Nearest-neighbour scaling keeps the pixel art
crisp (use --scale 2 for a 2x screenshot).
"""
import argparse
import struct
import sys
import zlib


def read_ppm(path):
    with open(path, "rb") as f:
        if f.readline().strip() != b"P6":
            raise ValueError("not a binary PPM (expected magic P6)")
        tokens = []
        buf = b""
        while len(tokens) < 3:
            ch = f.read(1)
            if not ch:
                raise ValueError("truncated PPM header")
            if ch == b"#":
                f.readline()
            elif ch.isspace():
                if buf:
                    tokens.append(buf)
                    buf = b""
            else:
                buf += ch
        width, height, maxval = (int(t) for t in tokens)
        if maxval != 255:
            raise ValueError("only 8-bit PPM files are supported")
        data = f.read(width * height * 3)
        if len(data) != width * height * 3:
            raise ValueError("truncated PPM pixel data")
    return width, height, data


def scale_nearest(width, height, rgb, factor):
    if factor <= 1:
        return width, height, rgb
    out = bytearray()
    for y in range(height):
        row = rgb[y * width * 3:(y + 1) * width * 3]
        row_out = bytearray()
        for x in range(width):
            row_out += row[x * 3:x * 3 + 3] * factor
        line = bytes(row_out)
        for _ in range(factor):
            out += line
    return width * factor, height * factor, bytes(out)


def write_png(path, width, height, rgb):
    def chunk(tag, payload):
        return (struct.pack(">I", len(payload)) + tag + payload +
                struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF))

    stride = width * 3
    raw = bytearray()
    for y in range(height):
        raw.append(0)  # filter type 0 (none)
        raw += rgb[y * stride:(y + 1) * stride]

    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(raw), 9))
    png += chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--scale", type=int, default=1,
                        help="nearest-neighbour integer scale factor")
    parser.add_argument("input", help="input PPM (P6) file")
    parser.add_argument("output", help="output PNG file")
    args = parser.parse_args()
    if args.scale < 1:
        parser.error("--scale must be >= 1")

    width, height, rgb = read_ppm(args.input)
    width, height, rgb = scale_nearest(width, height, rgb, args.scale)
    write_png(args.output, width, height, rgb)
    return 0


if __name__ == "__main__":
    sys.exit(main())
