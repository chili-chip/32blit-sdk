#!/usr/bin/env python3
"""Combine the loader's .uf2 with .blit files into one factory .uf2.

Each .blit is placed in flash at the given offset, where the loader finds it
as an installed game, so one copy in BOOTSEL mode sets up the whole console.

    factory-uf2.py blit-loader.uf2 vgc-os.blit@256 citsy.blit chili-beat.blit -o factory.uf2

Offsets are in KB and must be multiples of 64, at or above 256. A .blit with no
offset goes right after the one before it, on the next 64 KB boundary.
"""

import argparse
import struct
import sys

UF2_MAGIC_START0 = 0x0A324655
UF2_MAGIC_START1 = 0x9E5D5157
UF2_MAGIC_END = 0x0AB16F30
UF2_FLAG_FAMILY_ID = 0x00002000

FLASH_BASE = 0x10000000
PAYLOAD_SIZE = 256


def read_uf2(path):
    data = open(path, 'rb').read()
    if len(data) % 512:
        sys.exit(f'{path}: not a UF2 file')

    blocks = []
    for i in range(0, len(data), 512):
        start0, start1, flags, addr, size, _, _, family = struct.unpack_from('<8I', data, i)
        end, = struct.unpack_from('<I', data, i + 508)
        if start0 != UF2_MAGIC_START0 or start1 != UF2_MAGIC_START1 or end != UF2_MAGIC_END:
            sys.exit(f'{path}: bad UF2 block at {i}')
        blocks.append((flags, addr, data[i + 32:i + 32 + size], family))

    return blocks


BLOCK = 64 * 1024


def parse_blit_arg(arg, after):
    """(path, offset in bytes). `after` is where the previous .blit ends, or None for the first."""
    path, sep, offset = arg.rpartition('@')
    if not sep:
        if after is None:
            sys.exit(f'{arg}: the first .blit needs an offset, as file.blit@offset_kb')
        return arg, (after + BLOCK - 1) // BLOCK * BLOCK

    offset_kb = int(offset, 0)
    if offset_kb < 256 or offset_kb % 64:
        sys.exit(f'{arg}: offset should be >= 256 KB and a multiple of 64 KB')

    return path, offset_kb * 1024


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('loader', help='loader .uf2')
    parser.add_argument('blits', nargs='+', metavar='file.blit[@offset_kb]', help='.blit file and its flash offset in KB; with no offset, it goes after the one before')
    parser.add_argument('-o', '--output', required=True, help='output .uf2')
    args = parser.parse_args()

    blocks = read_uf2(args.loader)
    family = blocks[0][3]
    used = [(addr, addr + len(payload)) for _, addr, payload, _ in blocks]

    after = None
    for arg in args.blits:
        path, offset = parse_blit_arg(arg, after)
        data = open(path, 'rb').read()
        after = offset + len(data)
        start = FLASH_BASE + offset

        for lo, hi in used:
            if start < hi and lo < start + len(data):
                sys.exit(f'{path} overlaps something else at 0x{lo:08x}')
        used.append((start, start + len(data)))

        for i in range(0, len(data), PAYLOAD_SIZE):
            payload = data[i:i + PAYLOAD_SIZE].ljust(PAYLOAD_SIZE, b'\xff')
            blocks.append((UF2_FLAG_FAMILY_ID, start + i, payload, family))

    # one sequence numbered 0..n-1, so the bootloader takes it as a single image
    out = bytearray()
    for i, (flags, addr, payload, family) in enumerate(blocks):
        block = struct.pack('<8I', UF2_MAGIC_START0, UF2_MAGIC_START1, flags, addr, len(payload), i, len(blocks), family)
        block += payload.ljust(476, b'\x00')
        block += struct.pack('<I', UF2_MAGIC_END)
        out += block

    open(args.output, 'wb').write(out)


if __name__ == '__main__':
    main()
