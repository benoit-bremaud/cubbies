// Jealousy (social-group/05e): the PET codec (06), the sender rules S1–S4 over LoopbackRadio, one
// test per row of the receiver table on the Mood engine, and the Sulk effect mapping (02e).
#include <unity.h>

#include <initializer_list>

#include "FakeClock.h"
#include "LoopbackRadio.h"
#include "core/Choir.h"
#include "core/Identity.h"
#include "core/MoodEngine.h"
#include "core/PresenceCodec.h"
#include "core/SocialProtocol.h"
#include "core/StimulusRouter.h"

using cubbies::DecodeResult;
using cubbies::decodePet;
using cubbies::effects;
using cubbies::encodePet;
using cubbies::encodePresence;
using cubbies::kFrameSize;
using cubbies::Mac;
using cubbies::Mood;
using cubbies::MoodEffect;
using cubbies::MoodEngine;
using cubbies::MoodTally;
using cubbies::Pet;
using cubbies::Presence;
using cubbies::Prng;
using cubbies::Reaction;
using cubbies::SelfState;
using cubbies::SleepState;
using cubbies::SocialOutput;
using cubbies::SocialProtocol;
using cubbies::SoundId;
using cubbies::Stimulus;
using cubbies::test::Air;
using cubbies::test::FakeClock;
using cubbies::test::LoopbackRadio;
using E = MoodEngine;

namespace {
constexpr uint32_t kT0 = 1000;
constexpr uint32_t kTick = 40;
}  // namespace

void setUp() {}
void tearDown() {}

// — PET codec (06) —

static void test_pet_encode_matches_the_06_layout() {
  uint8_t out[kFrameSize];
  encodePet(Pet{5}, out);
  const uint8_t expected[kFrameSize] = {0x43, 0x42, 1, 0x04, 5, 0, 0, 0};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, out, kFrameSize);
}

static void test_pet_round_trips_every_palette_index() {
  for (uint8_t pal = 0; pal < cubbies::kPaletteSize; ++pal) {
    uint8_t buf[kFrameSize];
    encodePet(Pet{pal}, buf);
    Pet p{};
    TEST_ASSERT_EQUAL(DecodeResult::Ok, decodePet(buf, kFrameSize, p));
    TEST_ASSERT_EQUAL_UINT8(pal, p.paletteIndex);
  }
}

static DecodeResult decodePetWith(size_t offset, uint8_t value) {
  uint8_t buf[kFrameSize] = {0x43, 0x42, 1, 0x04, 1, 0, 0, 0};
  buf[offset] = value;
  Pet p{};
  return decodePet(buf, kFrameSize, p);
}

// Receiver row 9: the allowlist drops a malformed PET by reason.
static void test_pet_allowlist_drops_by_reason() {
  Pet p{};
  const uint8_t ok[kFrameSize] = {0x43, 0x42, 1, 0x04, 1, 0, 0, 0};
  TEST_ASSERT_EQUAL(DecodeResult::Ok, decodePet(ok, kFrameSize, p));
  TEST_ASSERT_EQUAL(DecodeResult::Len, decodePet(ok, kFrameSize - 1, p));
  TEST_ASSERT_EQUAL(DecodeResult::Len, decodePet(nullptr, kFrameSize, p));
  TEST_ASSERT_EQUAL(DecodeResult::Magic, decodePetWith(0, 0x00));
  TEST_ASSERT_EQUAL(DecodeResult::Version, decodePetWith(2, 2));
  TEST_ASSERT_EQUAL(DecodeResult::Type, decodePetWith(3, 0x03));
  TEST_ASSERT_EQUAL(DecodeResult::Range, decodePetWith(4, cubbies::kPaletteSize));
  TEST_ASSERT_EQUAL(DecodeResult::Range, decodePetWith(5, 1));
  TEST_ASSERT_EQUAL(DecodeResult::Range, decodePetWith(6, 1));
  TEST_ASSERT_EQUAL(DecodeResult::Range, decodePetWith(7, 1));
}

// — Over the radio: sender rules S1–S4 and the receiver's SocialOutput::pet —

namespace {
Mac mac(uint8_t last) { return Mac{{0x10, 0, 0, 0, 0, last}}; }

struct Cube {
  Cube(Air& air, uint8_t last, FakeClock& clock, uint8_t palette)
      : radio(air, mac(last), clock), prng(last, last), social(clock, radio, prng, mac(last)) {
    self.paletteIndex = palette;
  }
  LoopbackRadio radio;
  Prng prng;
  SocialProtocol social;
  SelfState self{static_cast<uint8_t>(0), true, SleepState::Awake, SleepState::Awake, Mood::Curious};

  // The main.cpp stroke wiring (02e): a choir when allowed, otherwise PET.
  void stroke() {
    if (!social.startChoir(self)) social.sendPet(self);
    social.tick(self);  // main.cpp ticks after the stroke: the choir enters its cooldown
  }
};

void announce(Cube& cube, uint8_t from) {
  uint8_t f[kFrameSize];
  encodePresence(Presence{1, 0, true}, f);
  cube.radio.inject(f, kFrameSize, mac(from));
  cube.social.tick(cube.self);
}

size_t choirFrames(const LoopbackRadio& r) {
  size_t n = 0;
  for (const auto& f : r.sent) {
    cubbies::ChoirStart c{};
    if (cubbies::decodeChoir(f.data(), f.size(), c) == DecodeResult::Ok) ++n;
  }
  return n;
}

size_t petFrames(const LoopbackRadio& r) {
  size_t n = 0;
  for (const auto& f : r.sent) {
    Pet p{};
    if (decodePet(f.data(), f.size(), p) == DecodeResult::Ok) ++n;
  }
  return n;
}
}  // namespace

// S1: the first stroke starts the choir and sends no PET; S2: the next one, during the choir
// cooldown, sends one PET carrying the petted cube's palette, and the receiver flags it.
static void test_s1_s2_choir_first_then_pet() {
  FakeClock clock(kT0);
  Air air;
  Cube a(air, 1, clock, 3);
  Cube b(air, 2, clock, 6);
  announce(a, 2);
  a.stroke();
  TEST_ASSERT_EQUAL_size_t(1, choirFrames(a.radio));
  TEST_ASSERT_EQUAL_size_t(0, petFrames(a.radio));
  b.social.tick(b.self);  // drains the CHOIR
  clock.advance(30000);
  announce(a, 2);
  a.stroke();
  TEST_ASSERT_EQUAL_size_t(1, choirFrames(a.radio));  // choir cooling down: PET instead
  TEST_ASSERT_EQUAL_size_t(1, petFrames(a.radio));
  Pet p{};
  for (const auto& f : a.radio.sent)
    if (decodePet(f.data(), f.size(), p) == DecodeResult::Ok) break;  // the only PET, wherever it sits
  TEST_ASSERT_EQUAL_UINT8(3, p.paletteIndex);
  TEST_ASSERT_TRUE(b.social.tick(b.self).pet);
  TEST_ASSERT_FALSE(b.social.tick(b.self).pet);  // one flag per frame
}

// S3: a stroke within T_pet_tx of the last PET sends nothing; at T_pet_tx it sends again.
static void test_s3_pet_sender_cooldown() {
  FakeClock clock(kT0);
  Air air;
  Cube a(air, 1, clock, 3);
  announce(a, 2);
  a.stroke();  // choir
  clock.advance(1000);
  a.stroke();
  TEST_ASSERT_EQUAL_size_t(1, petFrames(a.radio));
  clock.advance(SocialProtocol::kPetCooldownMs - 1);
  announce(a, 2);
  a.stroke();
  TEST_ASSERT_EQUAL_size_t(1, choirFrames(a.radio));  // still in the choir cooldown: sendPet ran
  TEST_ASSERT_EQUAL_size_t(1, petFrames(a.radio));
  clock.advance(1);
  a.stroke();
  TEST_ASSERT_EQUAL_size_t(2, petFrames(a.radio));
}

// S4: not Awake, or alone, nothing is sent.
static void test_s4_not_awake_or_alone_sends_nothing() {
  FakeClock clock(kT0);
  Air air;
  Cube a(air, 1, clock, 3);
  TEST_ASSERT_FALSE(a.social.sendPet(a.self));  // alone
  announce(a, 2);
  a.self.sleep = SleepState::Dozing;
  TEST_ASSERT_FALSE(a.social.sendPet(a.self));
  TEST_ASSERT_EQUAL_size_t(0, petFrames(a.radio));
  a.self.sleep = SleepState::Awake;
  TEST_ASSERT_TRUE(a.social.sendPet(a.self));
}

// Row 5: a PET received while not Awake is dropped, not kept for the wake.
static void test_row5_pet_while_not_awake_is_dropped() {
  FakeClock clock(kT0);
  Air air;
  Cube b(air, 2, clock, 6);
  uint8_t f[kFrameSize];
  encodePet(Pet{3}, f);
  b.radio.inject(f, kFrameSize, mac(1));
  b.self.sleep = SleepState::Asleep;
  TEST_ASSERT_FALSE(b.social.tick(b.self).pet);
  b.self.sleep = SleepState::Awake;
  TEST_ASSERT_FALSE(b.social.tick(b.self).pet);
}

// Row 6 (radio side): several PET frames in one tick raise one flag; row 9: a malformed one counts
// dropRange and raises none.
static void test_row6_row9_one_flag_per_tick_and_malformed_dropped() {
  FakeClock clock(kT0);
  Air air;
  Cube b(air, 2, clock, 6);
  uint8_t f[kFrameSize];
  encodePet(Pet{3}, f);
  b.radio.inject(f, kFrameSize, mac(1));
  b.radio.inject(f, kFrameSize, mac(3));
  const SocialOutput out = b.social.tick(b.self);
  TEST_ASSERT_TRUE(out.pet);
  f[7] = 1;
  b.radio.inject(f, kFrameSize, mac(1));
  TEST_ASSERT_FALSE(b.social.tick(b.self).pet);
  TEST_ASSERT_EQUAL_UINT32(1, b.social.stats().dropRange);
}

// Row 11: a cube that sings a choir (pending or within kChoirSingMs of its start) raises no flag.
static void test_row11_no_sulk_while_singing() {
  FakeClock clock(kT0);
  Air air;
  Cube b(air, 2, clock, 6);
  uint8_t f[kFrameSize];
  cubbies::encodeChoir(cubbies::ChoirStart{3, 0, cubbies::Choir::kLead}, f);
  b.radio.inject(f, kFrameSize, mac(1));
  encodePet(Pet{3}, f);
  b.radio.inject(f, kFrameSize, mac(1));
  TEST_ASSERT_TRUE(b.social.tick(b.self).choir.sing);  // the PET in the same drain is ignored
  clock.advance(cubbies::Choir::kChoirSingMs - 1);
  b.radio.inject(f, kFrameSize, mac(1));
  TEST_ASSERT_FALSE(b.social.tick(b.self).pet);
  clock.advance(1);
  b.radio.inject(f, kFrameSize, mac(1));
  TEST_ASSERT_TRUE(b.social.tick(b.self).pet);
}

// — Receiver rules on the Mood engine (05e rows 1–4, 6–8, 10) —

namespace {
struct Rig {
  Rig() : clock(kT0), engine(clock) {}
  FakeClock clock;
  MoodEngine engine;
  void idle(uint32_t ms) {
    for (uint32_t s = 0; s < ms; s += kTick) {
      clock.advance(kTick);
      engine.tick();
    }
  }
};
}  // namespace

// Row 1: calm and out of cooldown, a PET makes the cube sulk; the mood is unchanged (row 7).
static void test_row1_row7_calm_cube_sulks_mood_unchanged() {
  Rig r;
  r.idle(E::kJealousCalmMs);
  r.engine.stimulus(Stimulus::NewcomerSeen);  // not a calm break
  TEST_ASSERT_EQUAL(MoodEffect::Sulk, r.engine.jealousy());
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
}

// Row 7: the sulk plays the same in Annoyed and Happy, and leaves the mood as it was.
static void test_row7_sulk_in_any_mood() {
  Rig h;
  h.engine.stimulus(Stimulus::Tap);  // Curious → Happy
  h.clock.advance(E::kJealousCalmMs);
  TEST_ASSERT_EQUAL(MoodEffect::Sulk, h.engine.jealousy());
  TEST_ASSERT_EQUAL(Mood::Happy, h.engine.mood());
  Rig r;
  r.engine.stimulus(Stimulus::Loud);  // Curious → Annoyed
  TEST_ASSERT_EQUAL(Mood::Annoyed, r.engine.mood());
  r.clock.advance(E::kJealousCalmMs);
  TEST_ASSERT_EQUAL(MoodEffect::Sulk, r.engine.jealousy());
  TEST_ASSERT_EQUAL(Mood::Annoyed, r.engine.mood());
}

// Row 2: a gesture, a touch-down or a sound within T_calm_jealous — nothing, and the PET is not kept.
static void test_row2_cube_in_use_is_not_jealous() {
  Rig r;
  r.idle(E::kJealousCalmMs);
  r.clock.advance(E::kJealousCalmMs - 1000);
  r.engine.stimulus(Stimulus::Tap);
  r.clock.advance(E::kJealousCalmMs - 1);
  TEST_ASSERT_EQUAL(MoodEffect::None, r.engine.jealousy());
  for (const Stimulus s : {Stimulus::Voice, Stimulus::Loud}) {
    r.clock.advance(E::kJealousCalmMs);
    r.engine.stimulus(s);
    r.clock.advance(E::kJealousCalmMs - 1);
    TEST_ASSERT_EQUAL(MoodEffect::None, r.engine.jealousy());
  }
  r.clock.advance(E::kJealousCalmMs);
  r.engine.touchDown();
  r.clock.advance(E::kJealousCalmMs - 1);
  TEST_ASSERT_EQUAL(MoodEffect::None, r.engine.jealousy());
  r.clock.advance(1);
  TEST_ASSERT_EQUAL(MoodEffect::Sulk, r.engine.jealousy());  // a fresh PET once calm again
}

// Row 3: within T_jealous of the last sulk, nothing; at T_jealous, again.
static void test_row3_cooldown() {
  Rig r;
  r.idle(E::kJealousCalmMs);
  TEST_ASSERT_EQUAL(MoodEffect::Sulk, r.engine.jealousy());
  r.clock.advance(E::kJealousMs - 1);
  TEST_ASSERT_EQUAL(MoodEffect::None, r.engine.jealousy());
  r.clock.advance(1);
  TEST_ASSERT_EQUAL(MoodEffect::Sulk, r.engine.jealousy());
}

// Row 4: at most kJealousBudget sulks per kJealousWindowMs (ADR-0012), then free again.
static void test_row4_global_budget() {
  Rig r;
  r.idle(E::kJealousCalmMs);
  for (uint8_t i = 0; i < E::kJealousBudget; ++i) {
    TEST_ASSERT_EQUAL(MoodEffect::Sulk, r.engine.jealousy());
    r.clock.advance(E::kJealousMs);
  }
  TEST_ASSERT_EQUAL(MoodEffect::None, r.engine.jealousy());  // 4 in the last 4 min
  r.clock.advance(E::kJealousWindowMs - E::kJealousBudget * E::kJealousMs);
  TEST_ASSERT_EQUAL(MoodEffect::None, r.engine.jealousy());  // the first is exactly window old
  r.clock.advance(1);
  TEST_ASSERT_EQUAL(MoodEffect::Sulk, r.engine.jealousy());
}

// Row 6 (engine side): a second jealousy() in the same tick never sulks twice.
static void test_row6_at_most_one_sulk() {
  Rig r;
  r.idle(E::kJealousCalmMs);
  TEST_ASSERT_EQUAL(MoodEffect::Sulk, r.engine.jealousy());
  TEST_ASSERT_EQUAL(MoodEffect::None, r.engine.jealousy());
}

// Row 8: the sulk does not break the calm, start the G2 refractory, or feed the idle timer.
static void test_row8_sulk_leaves_calm_refractory_and_idle_untouched() {
  Rig r;
  r.idle(E::kBoredMs - E::kContagionMs);
  TEST_ASSERT_EQUAL(MoodEffect::Sulk, r.engine.jealousy());
  r.idle(E::kContagionMs);
  TEST_ASSERT_EQUAL(Mood::Bored, r.engine.mood());  // the idle timer ran on
  MoodTally t;
  t.votes[static_cast<uint8_t>(Mood::Happy)] = 1;
  r.engine.contagion(t);  // the hold starts now
  r.clock.advance(E::kContagionMs - kTick);
  r.engine.contagion(t);
  r.clock.advance(E::kJealousMs);  // out of the cooldown, the hold still running
  r.engine.contagion(t);
  TEST_ASSERT_EQUAL(Mood::Happy, r.engine.mood());  // first pull: no refractory, calm held
}

// Row 8 (calm): a sulk in the middle of a G2 hold does not delay the pull.
static void test_row8_sulk_does_not_break_the_calm() {
  Rig r;
  r.idle(E::kBoredMs);
  TEST_ASSERT_EQUAL(Mood::Bored, r.engine.mood());
  MoodTally t;
  t.votes[static_cast<uint8_t>(Mood::Happy)] = 1;
  r.engine.contagion(t);  // the hold starts now
  r.clock.advance(E::kContagionMs - kTick);
  TEST_ASSERT_EQUAL(MoodEffect::None, r.engine.contagion(t));
  TEST_ASSERT_EQUAL(MoodEffect::Sulk, r.engine.jealousy());
  r.clock.advance(kTick);
  r.engine.contagion(t);
  TEST_ASSERT_EQUAL(Mood::Happy, r.engine.mood());
}

// Row 4 across the wrap: a full budget older than the window is forgotten, never aliased.
static void test_budget_forgotten_after_a_long_quiet_spell() {
  Rig r;
  r.idle(E::kJealousCalmMs);
  for (uint8_t i = 0; i < E::kJealousBudget; ++i) {
    TEST_ASSERT_EQUAL(MoodEffect::Sulk, r.engine.jealousy());
    r.clock.advance(E::kJealousMs);
  }
  r.clock.advance(0xFFFFFFFFu - 2 * E::kJealousMs);  // last sulk ≈ 2^32 − 60 s ago
  TEST_ASSERT_EQUAL(MoodEffect::Sulk, r.engine.jealousy());
}

// Row 10: a wake (restart) keeps the cooldown and the budget.
static void test_row10_restart_keeps_cooldown() {
  Rig r;
  r.idle(E::kJealousCalmMs);
  TEST_ASSERT_EQUAL(MoodEffect::Sulk, r.engine.jealousy());
  r.engine.restart();
  r.clock.advance(E::kJealousCalmMs);
  TEST_ASSERT_EQUAL(MoodEffect::None, r.engine.jealousy());
}

// Clock wrap: the cooldown and the budget stay correct across 2^32 ms.
static void test_cooldown_across_the_clock_wrap() {
  FakeClock clock(0xFFFFFFFFu - E::kJealousCalmMs - 10);
  MoodEngine engine(clock);
  clock.advance(E::kJealousCalmMs);
  TEST_ASSERT_EQUAL(MoodEffect::Sulk, engine.jealousy());
  clock.advance(E::kJealousMs - 1);
  TEST_ASSERT_EQUAL(MoodEffect::None, engine.jealousy());
  clock.advance(1);
  TEST_ASSERT_EQUAL(MoodEffect::Sulk, engine.jealousy());
}

// 02e: the Sulk effect maps to the Sulk eyes and the grumble through the Sound gate.
static void test_sulk_effect_maps_to_eyes_and_grumble() {
  const cubbies::Route r = effects(MoodEffect::Sulk);
  TEST_ASSERT_TRUE(r.react);
  TEST_ASSERT_EQUAL(Reaction::Sulk, r.reaction);
  TEST_ASSERT_TRUE(r.sound);
  TEST_ASSERT_EQUAL(SoundId::Grumpy, r.soundId);
  TEST_ASSERT_FALSE(r.toMood);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_pet_encode_matches_the_06_layout);
  RUN_TEST(test_pet_round_trips_every_palette_index);
  RUN_TEST(test_pet_allowlist_drops_by_reason);
  RUN_TEST(test_s1_s2_choir_first_then_pet);
  RUN_TEST(test_s3_pet_sender_cooldown);
  RUN_TEST(test_s4_not_awake_or_alone_sends_nothing);
  RUN_TEST(test_row5_pet_while_not_awake_is_dropped);
  RUN_TEST(test_row6_row9_one_flag_per_tick_and_malformed_dropped);
  RUN_TEST(test_row11_no_sulk_while_singing);
  RUN_TEST(test_row1_row7_calm_cube_sulks_mood_unchanged);
  RUN_TEST(test_row7_sulk_in_any_mood);
  RUN_TEST(test_row2_cube_in_use_is_not_jealous);
  RUN_TEST(test_row3_cooldown);
  RUN_TEST(test_row4_global_budget);
  RUN_TEST(test_row6_at_most_one_sulk);
  RUN_TEST(test_row8_sulk_leaves_calm_refractory_and_idle_untouched);
  RUN_TEST(test_row8_sulk_does_not_break_the_calm);
  RUN_TEST(test_budget_forgotten_after_a_long_quiet_spell);
  RUN_TEST(test_row10_restart_keeps_cooldown);
  RUN_TEST(test_cooldown_across_the_clock_wrap);
  RUN_TEST(test_sulk_effect_maps_to_eyes_and_grumble);
  return UNITY_END();
}
