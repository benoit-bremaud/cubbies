// cubbies firmware entry point: builds the adapters, injects them into the lib behaviours and
// runs the 40 ms tick (03-component). Boot: Wi-Fi up, then the Prngs are seeded (ADR-0013),
// then the identity is resolved and the invitation window and the adoption ritual run
// (identity/02a, 02b); only then do the router, the sleep cycle and the social protocol tick.
// No policy here: main applies the Routes and IdentityOutputs the lib returns.
#include <Arduino.h>
#include <WiFi.h>
#include <esp_mac.h>
#include <esp_random.h>

#include "BuzzerAudio.h"
#include "EspNowRadio.h"
#include "I2sMic.h"
#include "MillisClock.h"
#include "NvsStorage.h"
#include "Ssd1331Display.h"
#include "Ttp223Touch.h"
#include "core/AdoptionRitual.h"
#include "core/GestureClassifier.h"
#include "core/Identity.h"
#include "core/InvitationWindow.h"
#include "core/MoodEngine.h"
#include "core/Prng.h"
#include "core/SleepCycle.h"
#include "core/SocialProtocol.h"
#include "core/SoundClassifier.h"
#include "core/SoundGate.h"
#include "core/StimulusRouter.h"
#include "eyes/EyesRenderer.h"
#include "pins.h"
#include "version.h"

using namespace cubbies;

namespace {

constexpr uint32_t kTickMs = 40;  // 25 fps (REQ-NF-02)

MillisClock clock_;
Ttp223Touch touch(pins::kTouch);
I2sMic mic(pins::kMicBclk, pins::kMicWs, pins::kMicSd);
Ssd1331Display display(pins::kDisplaySck, pins::kDisplayMosi, pins::kDisplayCs, pins::kDisplayDc);
BuzzerAudio audio(pins::kBuzzer);
EspNowRadio radio(clock_);
NvsStorage storage;

GestureClassifier gestures(clock_, touch);
SoundClassifier sounds(clock_, mic);
MoodEngine mood(clock_);
SleepCycle sleepCycle(clock_, mood);
SoundGate gate(clock_, audio);

// The consumers of randomness take their Prng at construction, and the Prngs can only be
// seeded once Wi-Fi is up, so they are built in setup(), once, and never freed.
struct Seeded {
  Prng eyesPrng;
  Prng socialPrng;
  EyesRenderer eyes;  // holds the 12 KB framebuffer: never on the loop stack
  SocialProtocol social;

  Seeded(uint64_t eyesSeed, uint64_t socialSeed)
      : eyesPrng(eyesSeed, 1),
        socialPrng(socialSeed, 2),
        eyes(clock_, eyesPrng, display),
        social(clock_, radio, socialPrng) {}
};
Seeded* seeded = nullptr;

Identity identity;
uint32_t nextTickMs = 0;

uint64_t randomSeed64() { return (static_cast<uint64_t>(esp_random()) << 32) | esp_random(); }

void waitForTick() {
  while (!isDue(clock_.nowMs(), nextTickMs)) delay(1);
  nextTickMs += kTickMs;
  // A tick that overran the budget does not pile up catch-up ticks.
  if (isDue(clock_.nowMs(), nextTickMs)) nextTickMs = clock_.nowMs() + kTickMs;
}

void apply(const IdentityOutput& out) {
  EyesRenderer& eyes = seeded->eyes;
  switch (out.frame) {
    case IdentityFrame::Keep: break;
    case IdentityFrame::Invitation: eyes.showInvitation(out.progress); break;
    case IdentityFrame::Ritual: eyes.showRitualFrame(out.colour); break;
    case IdentityFrame::Adoption: eyes.playAdoption(out.colour); break;
    case IdentityFrame::Error: eyes.showError(); break;
    case IdentityFrame::Clear: eyes.clearIdentityFrame(); break;
  }
  if (out.hasSound) gate.play(out.sound);
}

void applyEyesAndSound(const Route& r) {
  EyesRenderer& eyes = seeded->eyes;
  if (r.stopReaction) eyes.stopReaction();
  if (r.react) eyes.play(r.reaction);
  if (r.lids == Lids::Close) eyes.closeEyes();
  if (r.lids == Lids::Open) eyes.openEyes();
  if (r.sound) gate.play(r.soundId);
}

// Every SleepCycle call with an effect: the gate learns the new state first, so a wake sound
// is allowed and a doze cuts the sound in progress (F2 ordering), then the effect's Route.
void applySleep(SleepEffect e) {
  if (e == SleepEffect::None) return;
  gate.setSleepState(sleepCycle.state());
  seeded->eyes.setBrightness(sleepCycle.brightness());
  applyEyesAndSound(effects(e));
}

void applyMood(MoodEffect e) {
  if (e != MoodEffect::None) applyEyesAndSound(effects(e));
}

SleepEffect feedSleep(Gesture g) { return sleepCycle.gesture(g); }
SleepEffect feedSleep(SoundClass s) { return sleepCycle.sound(s); }

template <typename Input>
void applyRoute(const Route& r, Input input) {
  applyEyesAndSound(r);
  if (r.toMood) applyMood(mood.stimulus(r.moodStimulus));
  if (r.toSleep) applySleep(feedSleep(input));
}

void present() { seeded->eyes.tick(); }  // draws the frame and calls IDisplay::present

void runBoot() {
  EyesRenderer& eyes = seeded->eyes;
  eyes.setColour(paletteColour(identity.paletteIndex));

  InvitationWindow window(clock_, gestures);
  window.start();
  while (!window.entered() && !window.declined()) {
    waitForTick();
    gate.tick();  // first: a sound that just ended must not block this tick's sound
    apply(window.tick());
    present();
  }
  if (!window.entered()) return;

  AdoptionRitual ritual(clock_, storage);
  apply(ritual.start(identity));
  while (!ritual.isDone()) {
    waitForTick();
    gate.tick();
    apply(ritual.tick(gestures.tick()));
    present();
  }
  identity = ritual.identity();
  eyes.setColour(paletteColour(identity.paletteIndex));
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.printf("cubbies %s\n", kFirmwareVersion);

  // Wi-Fi first: esp_random() is only truly random once the RF is up (ADR-0013).
  WiFi.mode(WIFI_STA);
  if (!radio.begin()) Serial.println("radio: init failed, cube stays alone");
  seeded = new Seeded(randomSeed64(), randomSeed64());

  touch.begin();
  if (!storage.begin()) Serial.println("nvs: open failed, MAC colour only");
  if (!audio.begin()) Serial.println("audio: init failed, silent");
  if (!mic.begin()) Serial.println("mic: init failed");
  if (!display.begin()) Serial.println("display: init failed");

  Mac mac{};  // all zeros if the read fails: a defined, stable default colour
  if (esp_read_mac(mac.data(), ESP_MAC_WIFI_STA) != ESP_OK) {
    mac = Mac{};
    Serial.println("mac: read failed");
  }
  identity = resolveIdentity(mac, storage);

  gate.setSleepState(sleepCycle.state());
  seeded->eyes.setBrightness(sleepCycle.brightness());
  nextTickMs = clock_.nowMs();
  runBoot();
  radio.discardPending();
  mood.restart();
  gate.play(SoundId::WakeMelody);  // normal start (identity/02a)
}

void loop() {
  waitForTick();
  EyesRenderer& eyes = seeded->eyes;
  gate.tick();  // first: a sound that just ended must not block this tick's sounds

  const Gesture g = gestures.tick();
  if (g != Gesture::None) applyRoute(route(g, sleepCycle.state()), g);

  const SoundClass s = sounds.tick();
  if (s != SoundClass::None) applyRoute(route(s, sleepCycle.state()), s);

  const SocialOutput social = seeded->social.tick(SelfState{identity.paletteIndex, identity.adopted,
                                                            sleepCycle.state(),
                                                            sleepCycle.wakeOrigin(), mood.mood()});
  if (social.newcomer) {
    const Route r = routeNewcomer(sleepCycle.state());
    applyEyesAndSound(r);
    if (r.toMood) applyMood(mood.stimulus(r.moodStimulus));
  }

  applySleep(sleepCycle.tick());
  if (sleepCycle.state() == SleepState::Awake) applyMood(mood.tick());

  eyes.setMood(mood.mood());
  present();
}
