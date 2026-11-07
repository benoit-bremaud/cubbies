#!/bin/sh
# Make the PlatformIO penv trust the machine's locally installed CAs (#99). The pioarduino platform
# points SSL_CERT_FILE at the certifi bundle of ~/.platformio/penv before it runs `uv pip install`
# for its Python dependencies, so a TLS-inspecting endpoint agent (Aikido Device Protection, skill
# aikido-device-protection) whose CA lives only in the system store makes every board build fail
# with "invalid peer certificate: UnknownIssuer". This appends each CA of
# /usr/local/share/ca-certificates/*.crt that the bundle lacks; it is idempotent and runs before
# `pio run` (`just build`), because a certifi upgrade by the platform resets the bundle. Verification
# is never relaxed: the CAs added are the ones the operating system already trusts.
# Usage: pio-trust-local-ca.sh   (no-op without a penv or without local CAs)
set -eu
penv="${PLATFORMIO_CORE_DIR:-$HOME/.platformio}/penv"
bundle=$(find "$penv/lib" -path '*/site-packages/certifi/cacert.pem' 2>/dev/null | head -n 1)
[ -n "$bundle" ] || { echo "pio-trust-local-ca: no certifi bundle under $penv yet, nothing to do"; exit 0; }
added=0
for ca in /usr/local/share/ca-certificates/*.crt; do
  [ -f "$ca" ] || continue
  body=$(sed -n '2p' "$ca") # the first base64 line identifies the certificate
  grep -qF -- "$body" "$bundle" && continue
  { printf '\n# local CA %s (scripts/pio-trust-local-ca.sh)\n' "$(basename "$ca")"; cat "$ca"; } >> "$bundle"
  added=$((added + 1))
done
echo "pio-trust-local-ca: $added local CA(s) added to $bundle"
