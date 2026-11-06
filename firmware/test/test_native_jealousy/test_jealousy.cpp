// Jealousy (social-group/05e, G6 and G6a): the PET codec (06), the sender rules S1–S4 over
// LoopbackRadio, the input (petContext, presentSince), one test per row of the receiver table on
// the Mood engine and the JealousyRepertoire, and the effects mapping (02e).
#include <unity.h>

#include <initializer_list>

#include "FakeClock.h"
#include "LoopbackRadio.h"
#include "core/Choir.h"
#include "core/Identity.h"
#include "core/JealousyRepertoire.h"
#include "core/MoodEngine.h"
#include "core/PresenceCodec.h"
#include "core/SocialProtocol.h"
#include "core/StimulusRouter.h"

using cubbies::DecodeResult;
using cubbies::decodePet;
using cubbies::effects;
using cubbies::encodePet;
using cubbies::encodePresence;
using cubbies::EntryState;
using cubbies::kFrameSize;
using cubbies::Mac;
using cubbies::Mood;
using cubbies::MoodEffect;
using cubbies::MoodEngine;
using cubbies::MoodTally;
using cubbies::Neighbour;
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

// Row 11: a cube that sings a choir (within kSingMs of its start) raises no flag.
static void test_row11_no_sulk_while_singing() {
  FakeClock clock(kT0);
  Air air;
  Cube b(air, 2, clock, 6);
  uint8_t f[kFrameSize];
  cubbies::encodeChoir(cubbies::ChoirStart{3, 0, cubbies::Choir::kLead}, f);
  b.radio.inject(f, kFrameSize, mac(1));
  encodePet(Pet{3}, f);
  b.radio.inject(f, kFrameSize, mac(1));
  const SocialOutput first = b.social.tick(b.self);
  TEST_ASSERT_TRUE(first.choir.sing);
  TEST_ASSERT_FALSE(first.pet);  // the PET in the same drain is ignored
  clock.advance(cubbies::Choir::kSingMs - 1);
  b.radio.inject(f, kFrameSize, mac(1));
  TEST_ASSERT_FALSE(b.social.tick(b.self).pet);
  clock.advance(1);
  b.radio.inject(f, kFrameSize, mac(1));
  TEST_ASSERT_TRUE(b.social.tick(b.self).pet);
}

// — The input (05e): petContext from the first PET, presentSince (rows 6, 20–20e, 21) —

namespace {
void injectPet(Cube& cube, uint8_t from) {
  uint8_t f[kFrameSize];
  encodePet(Pet{3}, f);
  cube.radio.inject(f, kFrameSize, mac(from));
}

const Neighbour& entry(const Cube& cube, uint8_t last) {
  for (uint8_t i = 0; i < SocialProtocol::kCapacity; ++i)
    if (cube.social.neighbour(i).state != EntryState::Free && cube.social.neighbour(i).mac == mac(last))
      return cube.social.neighbour(i);
  TEST_FAIL_MESSAGE("no entry for that MAC");
  return cube.social.neighbour(0);
}
}  // namespace

// Input: neighbours counts the present entries, strokedAgeMs is the sender's presence age; row 6:
// with two PETs in one tick, the context comes from the first one drained.
static void test_pet_context_from_the_first_pet() {
  FakeClock clock(kT0);
  Air air;
  Cube b(air, 2, clock, 6);
  announce(b, 1);
  clock.advance(4000);
  announce(b, 3);
  clock.advance(1000);
  injectPet(b, 1);
  injectPet(b, 3);
  const SocialOutput out = b.social.tick(b.self);
  TEST_ASSERT_TRUE(out.pet);
  TEST_ASSERT_EQUAL_UINT8(2, out.petContext.neighbours);
  TEST_ASSERT_EQUAL_UINT32(5000, out.petContext.strokedAgeMs);
  injectPet(b, 3);
  injectPet(b, 1);
  TEST_ASSERT_EQUAL_UINT32(1000, b.social.tick(b.self).petContext.strokedAgeMs);
}

// Row 20e: a sender with no present entry is a newcomer (age 0); neighbours as counted.
static void test_row20e_unknown_sender_is_a_newcomer() {
  FakeClock clock(kT0);
  Air air;
  Cube b(air, 2, clock, 6);
  announce(b, 1);
  clock.advance(3000);
  injectPet(b, 9);
  const SocialOutput out = b.social.tick(b.self);
  TEST_ASSERT_TRUE(out.pet);
  TEST_ASSERT_EQUAL_UINT8(1, out.petContext.neighbours);
  TEST_ASSERT_EQUAL_UINT32(0, out.petContext.strokedAgeMs);
}

// Row 20: a new MAC is stored with presentSince = now, including one back after its entry was freed.
static void test_row20_new_mac_present_since_now() {
  FakeClock clock(kT0);
  Air air;
  Cube b(air, 2, clock, 6);
  announce(b, 1);
  TEST_ASSERT_EQUAL_UINT32(kT0, entry(b, 1).presentSince);
  clock.advance(SocialProtocol::kTtlMs + 1);
  b.social.tick(b.self);  // Absent
  clock.advance(SocialProtocol::kRegreetMs + 1);
  b.social.tick(b.self);  // freed past T_regreet
  announce(b, 1);
  TEST_ASSERT_EQUAL_UINT32(clock.nowMs(), entry(b, 1).presentSince);
}

// Row 20b: back from Absent within T_regreet, presentSince is kept.
static void test_row20b_back_within_regreet_keeps_present_since() {
  FakeClock clock(kT0);
  Air air;
  Cube b(air, 2, clock, 6);
  announce(b, 1);
  clock.advance(SocialProtocol::kTtlMs + 1);
  b.social.tick(b.self);
  TEST_ASSERT_EQUAL(EntryState::Absent, entry(b, 1).state);
  clock.advance(60000);
  announce(b, 1);
  TEST_ASSERT_EQUAL_UINT32(kT0, entry(b, 1).presentSince);
}

// Row 20c: back from Absent after more than T_regreet (greeted again), presentSince = now.
static void test_row20c_back_after_regreet_resets_present_since() {
  FakeClock clock(kT0);
  Air air;
  Cube b(air, 2, clock, 6);
  announce(b, 1);
  clock.advance(SocialProtocol::kTtlMs + 1);
  b.social.tick(b.self);  // Absent, lastSeen = kT0
  clock.advance(SocialProtocol::kRegreetMs);  // the frame arrives before expire() can free it
  announce(b, 1);
  TEST_ASSERT_EQUAL_UINT32(clock.nowMs(), entry(b, 1).presentSince);
}

// Row 20d: a refresh at age ≥ T_friend pins presentSince to now − T_friend (no wrap).
static void test_row20d_present_since_is_bounded() {
  FakeClock clock(kT0);
  Air air;
  Cube b(air, 2, clock, 6);
  announce(b, 1);
  while (clock.nowMs() - kT0 < cubbies::kFriendMs + 10000) {
    clock.advance(SocialProtocol::kPresenceMs);
    announce(b, 1);
  }
  TEST_ASSERT_EQUAL_UINT32(clock.nowMs() - cubbies::kFriendMs, entry(b, 1).presentSince);
  clock.advance(1000);
  injectPet(b, 1);
  TEST_ASSERT_EQUAL_UINT32(cubbies::kFriendMs + 1000, b.social.tick(b.self).petContext.strokedAgeMs);
}

// Row 21: a PET, accepted or dropped, is never relayed (the protocol has no storage port: nothing
// can reach NVS) and the drop counters are keyed by reason only.
static void test_row21_pet_never_relayed() {
  FakeClock clock(kT0);
  Air air;
  Cube b(air, 2, clock, 6);
  announce(b, 1);
  const size_t before = b.radio.sent.size();
  injectPet(b, 1);
  uint8_t bad[kFrameSize];
  encodePet(Pet{3}, bad);
  bad[5] = 1;
  b.radio.inject(bad, kFrameSize, mac(1));
  b.social.tick(b.self);
  TEST_ASSERT_EQUAL_size_t(0, petFrames(b.radio));
  TEST_ASSERT_LESS_OR_EQUAL_size_t(before + 1, b.radio.sent.size());  // a presence beacon at most
}

// — Receiver machine on the Mood engine (05e rows 1–12b, 16b, 19) —

namespace {
using cubbies::JealousReaction;
using cubbies::JealousStep;
using cubbies::JealousyRepertoire;
using cubbies::PetContext;
using JR = JealousReaction;

constexpr uint32_t kSecond = 1000;

bool isStep1(JR r) { return r == JR::SideGlance || r == JR::Stare || r == JR::TurnAway; }
bool isStep2(JR r) {
  return r == JR::Pout || r == JR::PuppyEyes || r == JR::MeToo || r == JR::FakeSleep || r == JR::Sulk;
}

struct Rig {
  Rig() : clock(kT0), engine(clock), rep(42) { clock.advance(E::kJealousCalmMs); }
  FakeClock clock;
  MoodEngine engine;
  JealousyRepertoire rep;
  PetContext ctx{1, cubbies::kFriendMs};
  JR pet(bool held = false) { return engine.jealousy(held, ctx, rep); }
  void idle(uint32_t ms) {
    for (uint32_t s = 0; s < ms; s += kTick) {
      clock.advance(kTick);
      engine.tick();
    }
  }
  // A notice then, 15 s later, a step 2 (rows 1, 1b); returns the step-2 reaction.
  JR pair() {
    TEST_ASSERT_TRUE(isStep1(pet()));
    clock.advance(15 * kSecond);
    const JR r = pet();
    TEST_ASSERT_TRUE(isStep2(r));
    return r;
  }
};
}  // namespace

// Row 1: a calm cube notices (a step-1 reaction), mood unchanged, no cooldown or budget entry.
static void test_row1_first_pet_is_noticed() {
  Rig r;
  TEST_ASSERT_TRUE(isStep1(r.pet()));
  TEST_ASSERT_TRUE(r.engine.noticed());
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
  r.clock.advance(kSecond);
  TEST_ASSERT_TRUE(isStep2(r.pet()));  // the notice started no cooldown
}

// Row 1b: a second PET within T_notice draws a step-2 reaction and returns to Indifferent; the
// cooldown and the budget entry run from it.
static void test_row1b_second_pet_within_notice_is_jealous() {
  Rig r;
  r.pair();
  TEST_ASSERT_FALSE(r.engine.noticed());
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
  r.clock.advance(E::kJealousMs - 1);
  TEST_ASSERT_EQUAL(JR::None, r.pet());
  r.clock.advance(1);
  TEST_ASSERT_TRUE(isStep1(r.pet()));  // back to Indifferent: a new notice
}

// Row 2: owner attention within T_calm_jealous — nothing; a Noticed cube returns to Indifferent.
static void test_row2_owner_attention_drops_the_notice() {
  for (const Stimulus s : {Stimulus::Tap, Stimulus::Voice, Stimulus::Loud}) {
    Rig r;
    TEST_ASSERT_TRUE(isStep1(r.pet()));
    r.clock.advance(kSecond);
    r.engine.stimulus(s);
    r.clock.advance(E::kJealousCalmMs - 1);
    TEST_ASSERT_EQUAL(JR::None, r.pet());
    TEST_ASSERT_FALSE(r.engine.noticed());
    r.clock.advance(1);
    TEST_ASSERT_TRUE(isStep1(r.pet()));  // a new notice, not a step 2
  }
  Rig t;
  t.engine.touchDown();
  t.clock.advance(E::kJealousCalmMs - 1);
  TEST_ASSERT_EQUAL(JR::None, t.pet());
  t.clock.advance(1);
  TEST_ASSERT_TRUE(isStep1(t.pet()));
}

// Row 2b: a pad held right now — nothing, however long the calm; a Noticed cube returns to
// Indifferent.
static void test_row2b_held_pad_drops_the_notice() {
  Rig r;
  r.clock.advance(10 * E::kJealousCalmMs);
  TEST_ASSERT_EQUAL(JR::None, r.pet(true));
  TEST_ASSERT_TRUE(isStep1(r.pet()));
  r.clock.advance(kSecond);
  TEST_ASSERT_EQUAL(JR::None, r.pet(true));
  TEST_ASSERT_FALSE(r.engine.noticed());
}

// Row 3: within T_jealous of the last step-2 reaction, nothing, and Noticed is not entered.
static void test_row3_cooldown_blocks_the_notice() {
  Rig r;
  r.pair();
  r.clock.advance(30 * kSecond);
  TEST_ASSERT_EQUAL(JR::None, r.pet());
  TEST_ASSERT_FALSE(r.engine.noticed());
}

// Row 4: kJealousBudget step-2 reactions in the window — a Noticed cube stays Noticed until
// T_notice elapses, then a new step 1.
static void test_row4_budget_keeps_the_notice() {
  Rig r;
  for (uint8_t i = 0; i < E::kJealousBudget; ++i) {
    r.pair();
    r.clock.advance(E::kJealousMs);
  }
  TEST_ASSERT_TRUE(isStep1(r.pet()));  // a notice costs no budget
  r.clock.advance(15 * kSecond);
  TEST_ASSERT_EQUAL(JR::None, r.pet());  // 4 step-2 reactions in the last 5 min
  TEST_ASSERT_TRUE(r.engine.noticed());
  r.clock.advance(E::kNoticeMs - 15 * kSecond);
  TEST_ASSERT_TRUE(isStep1(r.pet()));
}

// Row 4 across the wrap: a full budget older than every window is forgotten, never aliased.
static void test_budget_forgotten_after_a_long_quiet_spell() {
  Rig r;
  for (uint8_t i = 0; i < E::kJealousBudget; ++i) {
    r.pair();
    r.clock.advance(E::kJealousMs);
  }
  r.clock.advance(0xFFFFFFFFu - 2 * E::kJealousMs);  // last step 2 ≈ 2^32 − 60 s ago
  r.pair();
}

// Row 5: a restart (every wake) drops Noticed.
static void test_row5_restart_drops_the_notice() {
  Rig r;
  TEST_ASSERT_TRUE(isStep1(r.pet()));
  r.engine.restart();
  TEST_ASSERT_FALSE(r.engine.noticed());
  r.clock.advance(E::kJealousCalmMs);
  TEST_ASSERT_TRUE(isStep1(r.pet()));
}

// Row 6: a second call in the same tick never takes a second step.
static void test_row6_at_most_one_step_per_tick() {
  Rig r;
  TEST_ASSERT_TRUE(isStep1(r.pet()));
  TEST_ASSERT_EQUAL(JR::None, r.pet());
  r.clock.advance(kSecond);
  TEST_ASSERT_TRUE(isStep2(r.pet()));
  TEST_ASSERT_EQUAL(JR::None, r.pet());
}

// Row 7: the reaction plays in every mood and leaves the mood as it was.
static void test_row7_any_mood_unchanged() {
  for (const Stimulus s : {Stimulus::Tap, Stimulus::Loud}) {
    Rig r;
    r.engine.stimulus(s);  // Happy, Annoyed
    const Mood m = r.engine.mood();
    r.clock.advance(E::kJealousCalmMs);
    r.pair();
    TEST_ASSERT_EQUAL(m, r.engine.mood());
  }
  Rig b;
  b.idle(E::kBoredMs);
  TEST_ASSERT_EQUAL(Mood::Bored, b.engine.mood());
  b.pair();
  TEST_ASSERT_EQUAL(Mood::Bored, b.engine.mood());
}

// Row 8: no reaction feeds the idle timer or moves lastOwnerMs_ (Neglected still holds).
static void test_row8_reaction_leaves_owner_time_and_idle_untouched() {
  Rig r;
  r.idle(E::kBoredMs - E::kJealousCalmMs - 20 * kSecond);
  r.pair();
  r.clock.advance(5 * kSecond);
  r.idle(kTick);
  TEST_ASSERT_EQUAL(Mood::Bored, r.engine.mood());  // the idle timer ran on
  r.clock.advance(E::kLonelyMs);
  r.pair();
  TEST_ASSERT_TRUE(r.engine.contexts(r.ctx).neglected);
}

// Row 8 (calm): a reaction in the middle of a G2 hold does not delay the pull or start the
// refractory.
static void test_row8_reaction_does_not_break_the_calm() {
  Rig r;
  r.idle(E::kBoredMs);
  TEST_ASSERT_EQUAL(Mood::Bored, r.engine.mood());
  MoodTally t;
  t.votes[static_cast<uint8_t>(Mood::Happy)] = 1;
  r.engine.contagion(t);  // the hold starts now
  r.clock.advance(E::kContagionMs - kTick);
  TEST_ASSERT_EQUAL(MoodEffect::None, r.engine.contagion(t));
  TEST_ASSERT_TRUE(isStep1(r.pet()));
  r.clock.advance(kTick);
  r.engine.contagion(t);
  TEST_ASSERT_EQUAL(Mood::Happy, r.engine.mood());
}

// Row 10: a wake drops Noticed (row 5), sets lastOwnerMs_ to now and keeps the cooldown.
static void test_row10_restart_keeps_cooldown_and_resets_owner_time() {
  Rig r;
  r.clock.advance(E::kLonelyMs);
  TEST_ASSERT_TRUE(r.engine.contexts(r.ctx).neglected);
  r.pair();
  r.engine.restart();
  TEST_ASSERT_FALSE(r.engine.contexts(r.ctx).neglected);
  r.clock.advance(E::kJealousCalmMs);
  TEST_ASSERT_EQUAL(JR::None, r.pet());
}

// Rows 12, 12b: a PET T_notice or more after the notice, or after owner attention since the
// notice, starts a new step 1.
static void test_row12_row12b_stale_notice_restarts_at_step1() {
  Rig r;
  TEST_ASSERT_TRUE(isStep1(r.pet()));
  r.clock.advance(E::kNoticeMs - 1);
  TEST_ASSERT_TRUE(isStep2(r.pet()));
  r.clock.advance(E::kJealousMs);
  TEST_ASSERT_TRUE(isStep1(r.pet()));
  r.clock.advance(E::kNoticeMs);
  TEST_ASSERT_TRUE(isStep1(r.pet()));  // row 12
  r.clock.advance(kSecond);
  r.engine.stimulus(Stimulus::Voice);
  r.clock.advance(E::kJealousCalmMs + kSecond);
  TEST_ASSERT_TRUE(isStep1(r.pet()));  // row 12b
  r.clock.advance(kSecond);
  TEST_ASSERT_TRUE(isStep2(r.pet()));
}

// Row 16b: three step-2 reactions in the last 30 min, the last one 11 min ago: Weary (the ring is
// not cleared after 10 min); two are not enough; past T_weary it is gone.
static void test_row16b_weary_reads_the_ring_over_30_min() {
  Rig r;
  r.pair();
  r.clock.advance(E::kJealousMs);
  r.pair();
  TEST_ASSERT_FALSE(r.engine.contexts(r.ctx).weary);
  r.clock.advance(E::kJealousMs);
  r.pair();
  r.clock.advance(11 * 60 * kSecond);
  TEST_ASSERT_TRUE(r.engine.contexts(r.ctx).weary);
  r.clock.advance(E::kWearyMs);
  TEST_ASSERT_FALSE(r.engine.contexts(r.ctx).weary);
}

// Contexts: Bored, Crowd, Newcomer / Friend, Neglected from the engine's own state.
static void test_contexts_from_the_engine() {
  Rig r;
  auto c = r.engine.contexts(PetContext{1, cubbies::kFriendMs});
  TEST_ASSERT_TRUE(c.friend_);
  TEST_ASSERT_FALSE(c.newcomer || c.crowd || c.bored || c.neglected || c.weary);
  c = r.engine.contexts(PetContext{2, cubbies::kFriendMs - 1});
  TEST_ASSERT_TRUE(c.crowd && c.newcomer);
  TEST_ASSERT_FALSE(c.friend_);
  r.clock.advance(E::kLonelyMs - E::kJealousCalmMs - 1);
  TEST_ASSERT_FALSE(r.engine.contexts(r.ctx).neglected);
  r.clock.advance(1);
  TEST_ASSERT_TRUE(r.engine.contexts(r.ctx).neglected);
  r.engine.stimulus(Stimulus::NewcomerSeen);  // not owner attention
  TEST_ASSERT_TRUE(r.engine.contexts(r.ctx).neglected);
  r.idle(E::kBoredMs);
  TEST_ASSERT_TRUE(r.engine.contexts(r.ctx).bored);
}

// Row 19: a guard or the budget fails, no draw: the Prng sequence does not move.
static void test_row19_failed_guard_draws_nothing() {
  Rig r;
  JealousyRepertoire twin(42);
  TEST_ASSERT_EQUAL(JR::None, r.pet(true));  // row 2b
  r.engine.touchDown();
  TEST_ASSERT_EQUAL(JR::None, r.pet());  // row 2
  r.clock.advance(E::kJealousCalmMs);
  const JR a = r.pet();
  TEST_ASSERT_EQUAL(twin.draw(JealousStep::Notice, r.engine.contexts(r.ctx)), a);
}

// Clock wrap: the crescendo and the cooldown stay correct across 2^32 ms.
static void test_crescendo_across_the_clock_wrap() {
  FakeClock clock(0xFFFFFFFFu - E::kJealousCalmMs - 10);
  MoodEngine engine(clock);
  JealousyRepertoire rep(1);
  const PetContext ctx{1, 0};
  clock.advance(E::kJealousCalmMs);
  TEST_ASSERT_TRUE(isStep1(engine.jealousy(false, ctx, rep)));
  clock.advance(20 * kSecond);  // across the wrap
  TEST_ASSERT_TRUE(isStep2(engine.jealousy(false, ctx, rep)));
  clock.advance(E::kJealousMs - 1);
  TEST_ASSERT_EQUAL(JR::None, engine.jealousy(false, ctx, rep));
  clock.advance(1);
  TEST_ASSERT_TRUE(isStep1(engine.jealousy(false, ctx, rep)));
}

// — Repertoire: weights and draw (05e rows 14–18, worked example) —

namespace {
using cubbies::JealousContexts;
using cubbies::JealousWeights;

void assertWeights(JealousStep step, const JealousContexts& c, JR last, std::initializer_list<uint8_t> expected) {
  const JealousWeights w = cubbies::jealousWeights(step, c, last);
  TEST_ASSERT_EQUAL_UINT8(expected.size(), w.count);
  uint8_t i = 0;
  for (const uint8_t e : expected) TEST_ASSERT_EQUAL_UINT8(e, w.weight[i++]);
}

JealousContexts only(bool JealousContexts::*flag) {
  JealousContexts c;
  c.*flag = true;
  return c;
}

constexpr JealousStep kS1 = JealousStep::Notice;
constexpr JealousStep kS2 = JealousStep::Jealous;
}  // namespace

// The repertoire order of the 05e table, at each step.
static void test_weights_follow_the_table_order() {
  const JealousWeights s1 = cubbies::jealousWeights(kS1, {}, JR::None);
  const JR one[] = {JR::SideGlance, JR::Stare, JR::TurnAway};
  for (uint8_t i = 0; i < 3; ++i) TEST_ASSERT_EQUAL(one[i], s1.reaction[i]);
  const JealousWeights s2 = cubbies::jealousWeights(kS2, {}, JR::None);
  const JR two[] = {JR::Pout, JR::PuppyEyes, JR::MeToo, JR::FakeSleep, JR::Sulk};
  for (uint8_t i = 0; i < 5; ++i) TEST_ASSERT_EQUAL(two[i], s2.reaction[i]);
}

// Row 15: exact vectors on combined contexts.
static void test_row15_combined_contexts() {
  JealousContexts cf;
  cf.crowd = cf.friend_ = true;
  assertWeights(kS1, cf, JR::None, {5, 4, 2});
  JealousContexts cfn = cf;
  cfn.neglected = true;
  assertWeights(kS2, cfn, JR::None, {5, 4, 5, 1, 0});
  JealousContexts wn;
  wn.weary = wn.newcomer = true;
  assertWeights(kS2, wn, JR::None, {3, 2, 2, 5, 0});
}

// Row 16: each context alone applies its modifiers; Weary sets Sulk to 0.
static void test_row16_each_context_alone() {
  assertWeights(kS1, {}, JR::None, {3, 2, 2});
  assertWeights(kS2, {}, JR::None, {3, 2, 2, 1, 2});
  assertWeights(kS1, only(&JealousContexts::bored), JR::None, {3, 2, 3});
  assertWeights(kS2, only(&JealousContexts::bored), JR::None, {3, 5, 2, 1, 2});
  assertWeights(kS1, only(&JealousContexts::crowd), JR::None, {3, 4, 2});
  assertWeights(kS2, only(&JealousContexts::crowd), JR::None, {3, 2, 5, 1, 2});
  assertWeights(kS1, only(&JealousContexts::newcomer), JR::None, {3, 2, 4});
  assertWeights(kS2, only(&JealousContexts::newcomer), JR::None, {3, 2, 2, 1, 4});
  assertWeights(kS1, only(&JealousContexts::friend_), JR::None, {5, 2, 2});
  assertWeights(kS2, only(&JealousContexts::friend_), JR::None, {5, 2, 2, 1, 0});
  assertWeights(kS1, only(&JealousContexts::neglected), JR::None, {3, 2, 2});
  assertWeights(kS2, only(&JealousContexts::neglected), JR::None, {3, 4, 2, 1, 2});
  assertWeights(kS1, only(&JealousContexts::weary), JR::None, {5, 2, 2});
  assertWeights(kS2, only(&JealousContexts::weary), JR::None, {3, 2, 2, 5, 0});
}

// Row 16: a synthetic table whose modifier exceeds its base weight yields 0, not a negative weight.
static void test_row16_modifiers_clamp_at_zero() {
  cubbies::JealousRow rows[] = {{JR::Pout, 1, {-3, 0, 0, 0, 0, 0}}, {JR::Sulk, 2, {0, 0, 0, 0, 0, 0}}};
  const cubbies::JealousTable table{rows, 2};
  const JealousWeights w = cubbies::jealousWeights(table, only(&JealousContexts::bored), JR::None);
  TEST_ASSERT_EQUAL_UINT8(0, w.weight[0]);
  TEST_ASSERT_EQUAL_UINT8(2, w.weight[1]);
}

// Row 17 (weights side): the last reaction drawn at the step gets weight 0.
static void test_no_repeat_zeroes_the_last() {
  assertWeights(kS1, {}, JR::SideGlance, {0, 2, 2});
  assertWeights(kS2, {}, JR::Pout, {0, 2, 2, 1, 2});
}

// Row 14: the draw replays the reference sequence for (seed, contexts), each step with its own
// no-repeat memory, computed here by hand from the base weights and the draw rule.
static void test_row14_draw_replays_the_reference() {
  const uint8_t base1[] = {3, 2, 2};
  const JR order1[] = {JR::SideGlance, JR::Stare, JR::TurnAway};
  const uint8_t base2[] = {3, 2, 2, 1, 2};
  const JR order2[] = {JR::Pout, JR::PuppyEyes, JR::MeToo, JR::FakeSleep, JR::Sulk};
  JealousyRepertoire rep(7);
  Prng ref(7, JealousyRepertoire::kStream);
  JR last1 = JR::None;
  JR last2 = JR::None;
  auto expected = [&ref](const uint8_t* base, const JR* order, uint8_t n, JR& last) {
    uint8_t w[5];
    uint32_t total = 0;
    for (uint8_t i = 0; i < n; ++i) {
      w[i] = order[i] == last ? 0 : base[i];
      total += w[i];
    }
    uint32_t pick = ref.uniform(total);
    for (uint8_t i = 0; i < n; ++i) {
      if (pick < w[i]) return last = order[i];
      pick -= w[i];
    }
    return JR::None;
  };
  for (int i = 0; i < 50; ++i) {
    TEST_ASSERT_EQUAL(expected(base1, order1, 3, last1), rep.draw(kS1, {}));
    TEST_ASSERT_EQUAL(expected(base2, order2, 5, last2), rep.draw(kS2, {}));
  }
}

// Row 17: the same step drawn twice in a row never gives the same reaction.
static void test_row17_never_twice_in_a_row() {
  JealousyRepertoire rep(99);
  JealousContexts weary;
  weary.weary = true;
  for (const JealousStep step : {kS1, kS2}) {
    JR prev = JR::None;
    for (int i = 0; i < 500; ++i) {
      const JR r = rep.draw(step, weary);
      TEST_ASSERT_TRUE(r != JR::None && r != prev);
      prev = r;
    }
  }
}

// Row 18: 10 000 draws on one context, no-repeat off: each share within 2 points of its weight.
static void test_row18_frequencies_follow_the_weights() {
  JealousContexts c;
  c.crowd = true;
  const JealousWeights w = cubbies::jealousWeights(kS2, c, JR::None);
  Prng prng(2026, JealousyRepertoire::kStream);
  uint32_t hits[5] = {};
  constexpr uint32_t kDraws = 10000;
  for (uint32_t i = 0; i < kDraws; ++i) {
    const JR r = cubbies::drawWeighted(w, prng);
    for (uint8_t k = 0; k < w.count; ++k)
      if (w.reaction[k] == r) ++hits[k];
  }
  for (uint8_t k = 0; k < w.count; ++k) {
    // 2 percentage points of kDraws, in integers (Unity double precision is off).
    TEST_ASSERT_UINT32_WITHIN(kDraws / 50, w.weight[k] * kDraws / w.total(), hits[k]);
  }
}

// Worked example: Crowd + Friend + Weary after a MeToo → Pout 5, PuppyEyes 2, MeToo 0,
// FakeSleep 5, Sulk 0.
static void test_worked_example_weary_weights() {
  JealousContexts c;
  c.crowd = c.friend_ = c.weary = true;
  assertWeights(kS2, c, JR::MeToo, {5, 2, 0, 5, 0});
}

// — Effects (02e): each reaction maps to its eyes, step-2 sounds rank 2 through the Sound gate —

static void test_effects_map_each_reaction() {
  TEST_ASSERT_FALSE(effects(JR::None).react);
  TEST_ASSERT_FALSE(effects(JR::None).sound);
  const struct {
    JR in;
    Reaction eyes;
    bool sound;
    SoundId id;
  } rows[] = {
      {JR::SideGlance, Reaction::SideGlance, false, SoundId::Surprise},
      {JR::Stare, Reaction::Stare, false, SoundId::Surprise},
      {JR::TurnAway, Reaction::TurnAway, false, SoundId::Surprise},
      {JR::Pout, Reaction::Pout, true, SoundId::Hmpf},
      {JR::PuppyEyes, Reaction::PuppyEyes, true, SoundId::Whimper},
      {JR::MeToo, Reaction::MeToo, true, SoundId::Chirp},
      {JR::FakeSleep, Reaction::FakeSleep, false, SoundId::Surprise},
      {JR::Sulk, Reaction::Sulk, true, SoundId::Grumpy},
  };
  for (const auto& row : rows) {
    const cubbies::Route r = effects(row.in);
    TEST_ASSERT_TRUE(r.react);
    TEST_ASSERT_EQUAL(row.eyes, r.reaction);
    TEST_ASSERT_EQUAL(row.sound, r.sound);
    if (row.sound) {
      TEST_ASSERT_EQUAL(row.id, r.soundId);
      TEST_ASSERT_EQUAL_UINT8(2, cubbies::SoundGate::rank(r.soundId));
    }
    TEST_ASSERT_FALSE(r.toMood);
  }
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
  RUN_TEST(test_pet_context_from_the_first_pet);
  RUN_TEST(test_row20e_unknown_sender_is_a_newcomer);
  RUN_TEST(test_row20_new_mac_present_since_now);
  RUN_TEST(test_row20b_back_within_regreet_keeps_present_since);
  RUN_TEST(test_row20c_back_after_regreet_resets_present_since);
  RUN_TEST(test_row20d_present_since_is_bounded);
  RUN_TEST(test_row21_pet_never_relayed);
  RUN_TEST(test_row1_first_pet_is_noticed);
  RUN_TEST(test_row1b_second_pet_within_notice_is_jealous);
  RUN_TEST(test_row2_owner_attention_drops_the_notice);
  RUN_TEST(test_row2b_held_pad_drops_the_notice);
  RUN_TEST(test_row3_cooldown_blocks_the_notice);
  RUN_TEST(test_row4_budget_keeps_the_notice);
  RUN_TEST(test_budget_forgotten_after_a_long_quiet_spell);
  RUN_TEST(test_row5_restart_drops_the_notice);
  RUN_TEST(test_row6_at_most_one_step_per_tick);
  RUN_TEST(test_row7_any_mood_unchanged);
  RUN_TEST(test_row8_reaction_leaves_owner_time_and_idle_untouched);
  RUN_TEST(test_row8_reaction_does_not_break_the_calm);
  RUN_TEST(test_row10_restart_keeps_cooldown_and_resets_owner_time);
  RUN_TEST(test_row12_row12b_stale_notice_restarts_at_step1);
  RUN_TEST(test_row16b_weary_reads_the_ring_over_30_min);
  RUN_TEST(test_contexts_from_the_engine);
  RUN_TEST(test_row19_failed_guard_draws_nothing);
  RUN_TEST(test_crescendo_across_the_clock_wrap);
  RUN_TEST(test_weights_follow_the_table_order);
  RUN_TEST(test_row15_combined_contexts);
  RUN_TEST(test_row16_each_context_alone);
  RUN_TEST(test_row16_modifiers_clamp_at_zero);
  RUN_TEST(test_no_repeat_zeroes_the_last);
  RUN_TEST(test_row14_draw_replays_the_reference);
  RUN_TEST(test_row17_never_twice_in_a_row);
  RUN_TEST(test_row18_frequencies_follow_the_weights);
  RUN_TEST(test_worked_example_weary_weights);
  RUN_TEST(test_effects_map_each_reaction);
  return UNITY_END();
}
