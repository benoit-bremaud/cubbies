"""Checks the second method's parser and its sensitivity (run by run.sh; plain asserts, no pytest)."""

from __future__ import annotations

from crosscheck import Checker, check_i2c, num, rows

HEADER = "Rpull Vrail Cbus | tr30-70 [ns] | Isink@VOL0.4 [mA]\n"


def test_num() -> None:
    for text, expected in {
        "0.6m": 6e-4,
        "100p": 1e-10,
        "9E-08": 9e-8,
        "-0.115": -0.115,
        "4.7k": 4700,
    }.items():
        assert abs(num(text) - expected) <= abs(expected) * 1e-9, text
    for bad in ("1meg", "abc", ""):
        try:
            num(bad)
        except ValueError:
            continue
        raise AssertionError(f"accepted {bad!r}")


def test_rows_counts_and_skips() -> None:
    table = HEADER + "4700 3.3 100p | 397.9 | 0.617\nnoise | x\n1 2 | 3\n"
    assert rows(table, 5, 1) == [[4700, 3.3, 1e-10, 397.9, 0.617]]
    try:
        rows(table, 5, 2)
    except SystemExit:
        return
    raise AssertionError("a missing row passed")


def test_agree_catches_a_two_percent_error() -> None:
    tr_ns = 4700 * 100e-12 * 0.8472978603872037 * 1e9
    good = HEADER + f"4700 3.3 100p | {tr_ns} | {2.9 / 4.7}\n"
    bad = HEADER + f"4700 3.3 100p | {tr_ns * 1.02} | {2.9 / 4.7}\n"
    c = Checker()
    check_i2c(c, good, expect=1)
    assert c.failures == 0
    check_i2c(c, bad, expect=1)
    assert c.failures == 1


if __name__ == "__main__":
    test_num()
    test_rows_counts_and_skips()
    test_agree_catches_a_two_percent_error()
    print("crosscheck parser OK")
