# Golden images

`test_native_eyes` compares each rendered state with the PNG of the same name here (exact
pixel match, the rendering is deterministic). On a mismatch the test writes `<name>.actual.png`
next to the reference (ignored by git) and fails with the pixel count.

After an intended visual change, from `firmware/`:

```bash
CUBBIES_UPDATE_GOLDEN=1 pio test -e native
```

then `python3 tools/golden_sheet.py` (needs Pillow) and attach the before/after `sheet.png`
to the pull request (ADR-0014). Regeneration is refused when `CI` is set. Keep the `names`
list of the sheet script in sync with the files written by the test.
