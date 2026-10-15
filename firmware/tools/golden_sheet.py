#!/usr/bin/env python3
"""Contact sheet of the golden eye images, upscaled x4 for PR review (needs Pillow).

Usage: golden_sheet.py [out.png]  (default: test/golden/sheet.png). Keep `names` in sync with
the golden files written by test_native_eyes; missing files are skipped silently.
"""
import os, sys
from PIL import Image, ImageDraw

names = ["curious", "happy", "bored", "tired", "annoyed", "closed", "blink-1", "blink-2", "blink-3", "blink-4",
         "touchdown", "surprise", "touch-to-surprise", "joy-1", "joy-2", "lookup",
         "yawn-1", "yawn-2", "wake-1", "wake-2", "greet-1", "greet-2", "purr",
         "invitation-empty", "invitation-full", "ritual", "adoption-1", "adoption-2", "error"]
scale, pad, cols = 4, 12, 4
w, h = 96 * scale, 64 * scale
rows = (len(names) + cols - 1) // cols
sheet = Image.new("RGB", (cols * (w + pad) + pad, rows * (h + pad + 18) + pad), (40, 40, 40))
draw = ImageDraw.Draw(sheet)
root = os.path.join(os.path.dirname(__file__), "..", "test", "golden")
for i, n in enumerate(names):
    path = os.path.join(root, f"{n}.png")
    if not os.path.exists(path):
        continue
    im = Image.open(path).resize((w, h), Image.NEAREST)
    x, y = pad + (i % cols) * (w + pad), pad + (i // cols) * (h + pad + 18)
    sheet.paste(im, (x, y + 18))
    draw.text((x, y + 2), n, fill=(230, 230, 230))
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(root, "sheet.png")
sheet.save(out)
print(out)
