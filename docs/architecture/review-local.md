# Local review — cubbies (brick F4 social)

- Date: 2026-10-15
- Commit: 74618f0
- Scope: branch diff origin/main...feat/f4-social
- Checks run: design · docs · tests · security · correctness
- Verdict: fix Must Have first  ·  Must 1 · Should 11 · Nice 6

## 🔴 Must Have
- `firmware/test/test_native_social/test_social.cpp:247` `[tests]` · the 05a row "Waiting refresh" is untested (the entry is already Greeted) · hold the cooldown, hear neighbour 2 twice, assert Waiting + new mood + lastSeen

## 🟡 Should Have
- `firmware/lib/cubbies_core/ports/IRadio.h:10` `[design]` · port includes core/Identity.h (inverted dependency, drags eyes) · `ports/Mac.h` with Mac and kFrameSize, included by IRadio, Identity and the codec
- `firmware/lib/cubbies_core/core/SocialProtocol.cpp:24` `[design]` · receive() does three jobs · extract findSlot()
- `firmware/lib/cubbies_core/core/SocialProtocol.cpp:69` `[correctness]` · an entry absent for > 49.7 days aliases as fresh, owed re-greet lost · clear greeted in expire() once absent > T_regreet, test
- `firmware/lib/cubbies_core/core/SocialProtocol.cpp:84` `[correctness]` · the budget ring aliases after 49.7 days without greetings · reset the ring when the newest greeting is ≥ window old, test
- `firmware/lib/cubbies_core/ports/IRadio.h:2` + 06 `[docs]` · dropLen counted in adapter and lib · document the lib check as defence in depth
- `test_social.cpp:224` `[tests]` · Free → Waiting does not assert the state · assert Waiting
- `test_social.cpp:276` `[tests]` · "lowest waiting slot" unpinned · distinct palettes, assert order
- `test_social.cpp:268` `[tests]` · cooldown boundary unpinned · exact boundary test
- `test_social.cpp:297` `[tests]` · budget boundary unpinned (05a says "more than 10 min") · exact boundary test, code aligned to strict
- `test_social.cpp:340` `[tests]` · T_regreet boundary unpinned · exact and +1 tick tests
- `test_social.cpp:426` `[tests]` · flood test passes with 0 greetings · assert == budget, 4 per tick over several ticks, dropFull > 0

## 🔵 Nice to Have
- `SocialProtocol.h:34` `[design]` · six drop counters could be an array indexed by DecodeResult
- `firmware/test/support/LoopbackRadio.h:44` `[design|correctness]` · len clamp then overwrite; memcpy from null with len 0
- `SocialProtocol.h:59` `[correctness]` · neighbour(slot) unchecked · document precondition
- `test_social.cpp:422` `[tests]` · flood MACs include the cube's own MAC
- `test_social.cpp:380` `[tests]` · eviction test could assert the newcomer state and slot of 2
- `SocialProtocol.h:1` `[docs]` · header could mention stats()

## ⚪ Disagree / Intentional
- `SocialProtocol.cpp:83` `[design]` · greeting emitted while not Awake · the router drops it (02-sequence-presence)
- `ports/IRadio.h` `[design]` · RxFrame.nowMs stamped by the adapter · 05a, 03-component
- `LoopbackRadio.h` `[tests]` · unbounded queue · the 8-slot policy belongs to EspNowRadio (06)

## Per-dimension summary
- Design: conformant, one inverted include, one long method · Docs: consistent, dropLen double count to state · Tests: every row but one covered, boundaries loose · Security: allowlist complete, flood bounded, no PII logged · Correctness: two 2^32 aliasing edge cases
