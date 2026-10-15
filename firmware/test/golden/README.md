# Golden images

`test_native_eyes` compares each rendered state with the raw RGB565 dump of the same name
(`<name>.rgb565`, exact match, the rendering is deterministic). The PNG next to it is written by
the same regeneration for humans and GitHub (our own minimal encoder, `test/support/PngWriter.h`,
no third-party code). On a mismatch the test writes `<name>.actual.png` next to the reference
(ignored by git) and fails with the pixel count.

After an intended visual change, from `firmware/`:

```bash
CUBBIES_UPDATE_GOLDEN=1 pio test -e native
```

then `python3 tools/golden_sheet.py` (needs Pillow) and attach the before/after `sheet.png`
to the pull request (ADR-0014). Regeneration is refused when `CI` is set. Keep the `names`
list of the sheet script in sync with the files written by the test.
