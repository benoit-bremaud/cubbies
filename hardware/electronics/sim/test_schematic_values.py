"""Checks the value parser that feeds every bench (run by run.sh; plain asserts, no pytest)."""

from __future__ import annotations

import time

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
        "100p": 1e-10,
        "10kΩ": 1e4,
        "4.7µF": 4.7e-6,
        "1.5k": 1500,
    }
    for text, expected in cases.items():
        assert abs(si_value(text) - expected) <= expected * 1e-9, text
    for bad in ("1 uF", "2.2 k", "abc", "10kk", "10kΩΩ", "1.5.5k"):
        try:
            si_value(bad)
        except ValueError:
            continue
        raise AssertionError(f"accepted {bad!r}")


def test_si_value_linear_time() -> None:
    """A long unreadable value fails fast: the parser regex must not backtrack (python:S8786)."""
    start = time.perf_counter()
    try:
        si_value("1" * 20000 + "x")
    except ValueError:
        pass
    assert time.perf_counter() - start < 0.5, "parser regex backtracks super-linearly"


if __name__ == "__main__":
    test_si_value()
    test_si_value_linear_time()
    print("schematic_values parser OK")
