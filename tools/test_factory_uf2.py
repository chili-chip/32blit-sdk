"""Tests for factory-uf2.py: python3 -m unittest tools/test_factory_uf2.py"""

import importlib.util
import os
import struct
import subprocess
import sys
import tempfile
import unittest

HERE = os.path.dirname(__file__)
spec = importlib.util.spec_from_file_location('factory_uf2', os.path.join(HERE, 'factory-uf2.py'))
factory = importlib.util.module_from_spec(spec)
spec.loader.exec_module(factory)


def write_uf2(path, addr, data, family=0xE48BFF59):
    out = bytearray()
    blocks = [data[i:i + 256] for i in range(0, len(data), 256)]
    for i, payload in enumerate(blocks):
        out += struct.pack('<8I', factory.UF2_MAGIC_START0, factory.UF2_MAGIC_START1, factory.UF2_FLAG_FAMILY_ID,
                           addr + i * 256, len(payload), i, len(blocks), family)
        out += payload.ljust(476, b'\x00') + struct.pack('<I', factory.UF2_MAGIC_END)
    open(path, 'wb').write(out)


class FactoryUf2Tests(unittest.TestCase):
    def run_tool(self, *blits):
        tmp = tempfile.mkdtemp()
        loader = os.path.join(tmp, 'loader.uf2')
        write_uf2(loader, factory.FLASH_BASE, b'L' * 512)
        args = []
        for name, size, offset in blits:
            path = os.path.join(tmp, name)
            open(path, 'wb').write(b'B' * size)
            args.append(f'{path}@{offset}' if offset is not None else path)
        output = os.path.join(tmp, 'out.uf2')
        subprocess.run([sys.executable, os.path.join(HERE, 'factory-uf2.py'), loader, *args, '-o', output], check=True)
        return [addr for _, addr, _, _ in factory.read_uf2(output)]

    def test_a_blit_without_an_offset_goes_after_the_last_one_on_a_64_kb_boundary(self):
        addrs = self.run_tool(('home.blit', 70 * 1024, 256), ('handler.blit', 1024, None))
        base = factory.FLASH_BASE
        self.assertIn(base + 256 * 1024, addrs)
        # home.blit ends at 326 KB, so the next one starts at 384 KB.
        self.assertIn(base + 384 * 1024, addrs)
        self.assertNotIn(base + 326 * 1024 + 1024, addrs)

    def test_the_first_blit_needs_an_offset(self):
        with self.assertRaises(SystemExit):
            factory.parse_blit_arg('home.blit', None)


if __name__ == '__main__':
    unittest.main()
