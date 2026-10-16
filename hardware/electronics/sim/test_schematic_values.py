"""Checks the value parser that feeds every bench (run by run.sh; plain asserts, no pytest)."""

from __future__ import annotations

from schematic_values import si_value


def test_si_value() -> None:
    cases = {
        "10k": 1e4,
        "4k7": 4700,
        "1uF X7R": 1e-6,
        "82": 82,
        "100k": 1e5,
        "2.2k": 2200,
        "1µF": 1e-6,
        "100n": 1e-7,
        "1M": 1e6,
    }
    for text, expected in cases.items():
        assert abs(si_value(text) - expected) <= expected * 1e-9, text
    for bad in ("1 uF", "abc", "10kk"):
        try:
            si_value(bad)
        except SystemExit:
            continue
        raise AssertionError(f"accepted {bad!r}")


if __name__ == "__main__":
    test_si_value()
    print("schematic_values parser OK")
