# SMS Module Test — Results Log

**Module:** SIM800L V2.2 (UNV), 5 V board
**Board:** Arduino Uno
**Recipient tested:** `+639169751409`
**Date tested:** 30–31 July 2026
**Tested by:** Glenn
**Room / location:** ____________ *(fill in — 2G coverage is location-specific)*

---

## ✅ HEADLINE RESULT

**The module sends real 2G SMS.** Confirmed twice by hand on 31 July 2026, the second
returning `+CMGS: 184` followed by `OK`, with the text arriving on the phone.

That single success proves, all at once:

- the SIM has **2G coverage** in this location
- the SIM has **credit**
- the **SMSC** is set correctly
- the **wiring and common ground** are right
- the **logic levels** work with no level shifter (V2.2 is 5 V-logic)
- the module itself is **not faulty**

Everything that remains is power stability and software.

---

## Test status

| Test | What it proves | Status |
|---|---|---|
| Pre-check | SIM works on 2G in a normal phone | ☐ not formally done — proven indirectly by the successful send |
| **A** — power + status LED | Module alive, joined the network | ✅ **PASS** — settled to one blink every ~3 s |
| **B** — manual AT commands | Arduino can drive it; it can send | ✅ **PASS** — real text received, `+CMGS: 184` |
| **C** — automatic self-test | Same, hands-free, repeatable | ⬜ **NOT YET RUN** — next step |
| **D** — battery / laptop off | Alerts work with no computer | ⬜ **NOT YET RUN** — blocked on the power-bank cutoff |

---

## Still to confirm (blanks that matter)

These were never written down and should be, because they decide whether the build is
reliable or merely lucky:

| Item | Value |
|---|---|
| Wall adapter **amp rating** | ____________ ⚠️ **need 2 A minimum** |
| Network (Globe / TM / Smart / TNT) | ____________ |
| `AT+CSQ` reading | `+CSQ: ______` *(want above 10)* |
| `AT+CREG?` reading | ____________ *(want `0,1` or `0,5`)* |
| `AT+CSCA?` reading | ____________ |
| Capacitor **voltage rating** fitted | ______ V *(want ≥ 10 V)* |
| Capacitance fitted | ______ µF |
| Is module power still routed through the breadboard? | ☐ yes ☐ no — **should be no** |

---

# FAULT HISTORY

Every problem hit on this build, what caused it, and how it was found. This is the part
worth having for the defence — it shows the debugging was systematic, not lucky.

---

## Fault 1 — module powered on, off, then on again (repeating), and the capacitor died

**Symptom.** The module cycled on and off continuously. The 1000 µF capacitor stopped
working.

**Cause.** The capacitor was fitted **backwards**. A reverse-biased electrolytic does not
store charge — it conducts, increasingly, until it is effectively a short across the
supply. The power bank's over-current protection tripped, waited, and retried, producing
the on/off/on cycle. The capacitor destroyed itself in the process.

**One fault, both symptoms.**

**How it was found.** The pairing of a reboot loop *with* a dead capacitor points almost
uniquely at reversed polarity. The confirming test is to remove the capacitor entirely:
if the module then powers up and stays up, the capacitor was the fault.

**Fix.** New capacitor, **stripe leg to GND**, rated ≥ 10 V.

**Lesson.** The stripe on an electrolytic marks the *negative* leg. On an uncut part the
longer leg is positive. A capacitor that has vented, bulged or leaked is scrap.

---

## Fault 2 — everything worked, then the 5 V 3 A power bank switched itself off

**Symptom.** The module joined the network — LED blinking once every ~3 s — and then
after a short, repeatable time the power bank turned its own output off.

**Cause.** **Low-current auto-cutoff.** Nearly every power bank shuts down below roughly
50–100 mA, assuming whatever it was charging has finished. A registered SIM800L idles at
only 10–20 mA. The 3 A rating was never the issue.

Note the timing: while the module was still *searching* it transmitted hard enough to
stay above the threshold. It only fell below once it had **successfully joined** — so
this symptom was, perversely, evidence the module was working.

**Fix applied.** Switched to a **wall adapter** for bench testing, removing the bank's
cutoff logic from the picture entirely.

**Still outstanding for Test D.** The demo needs battery power, so before then: use the
bank's "always on" mode (often a double-press), or power the Arduino from the same bank
so the combined draw stays above the threshold, or fit a **47 Ω 1 W** bleeder resistor
across the module's supply (~106 mA).

---

## Fault 3 — `AT+CMGS` returned a bare `ERROR`

**Symptom.** Two different failures that both printed only `ERROR`:

1. `>` prompt appeared, message body accepted, then `ERROR`
2. `ERROR` **immediately, with no `>` prompt at all**

**Why the second one mattered.** `AT+CMGF=1` had already succeeded. If `AT+CMGS` then
errors without prompting, text mode is gone — and nothing typed had changed it. That
means **the module reset itself**, which points straight at power.

**Fix.** `AT+CMEE=2` was enabled so the module reports `+CMS ERROR: <code>` instead of a
bare `ERROR`. `AT+CMGF=1` re-issued after each reset.

**Lesson.** `ERROR` alone is not a diagnosis. Turn on verbose errors *first*, then debug.
A full `+CMS ERROR` code table is now in `AT_COMMANDS.md`.

---

## Fault 4 — stuck at the `>` prompt, no message ever sent

**Symptom.** `AT+CMGS="+639169751409"` gave the `>` prompt, text was typed, Enter pressed
— and no message ever arrived.

**Cause.** Not a fault at all: `>` is a **mode**. The module has stopped listening for
commands and treats *everything* typed as message body, including AT commands typed by
mistake. Pressing Enter only adds another line. Nothing sends until **Ctrl+Z**.

**Fix.** Type `END` on its own line — the passthrough sketch converts it to Ctrl+Z,
because the Arduino IDE's Serial Monitor cannot type that character.

**Tooling added.** `CANCEL` was added to the sketch. It sends ESC, abandoning a half-typed
message and returning to normal AT commands. Before this, the only way out of the `>`
prompt was resetting the module — a bad position when everything typed silently becomes
message text.

---

## Fault 5 — module restarted repeatedly after a successful send

**Symptom.** After a message went out successfully, the module dropped to a 1-second
blink (searching) and restarted roughly every 10 seconds.

**Cause.** **Brownout on the transmit burst.** Searching for a network is the module's
hungriest state — it transmits at full power repeatedly, drawing up to 2 A in bursts.
A supply that cannot hold 5 V under that load creates a self-feeding trap:

> searching → full-power bursts → supply sags → reset → searching again → …

It never stayed alive long enough to re-register. Before the send it was already
registered and sipping ~20 mA, so the weak supply had never been tested.

**Status: resolved in practice — sends now succeed.** But it is not proven fixed. If it
recovered because conditions were briefly favourable rather than because the supply path
changed, **it will come back**, most likely during the demo.

**Do before the demo.** Run `5Vin` and `GND` as a **short, thick, direct pair from the
adapter to the module**, bypassing the breadboard. Breadboard spring contacts are rated
~1 A each and dupont wire is thin — at 2 A the voltage is lost before it reaches the
module. Capacitor twisted or soldered onto the module's own terminals, and more bulk
capacitance (2200 µF, or a second 1000 µF in parallel) costs nothing and buys headroom.

---

## Fault 6 — the passthrough sketch corrupted the message body *(tooling bug)*

**Symptom.** Messages would have gone out reading `SupEND` rather than `Sup`.

**Cause.** A bug in `sim800l_passthrough.ino`: keystrokes were forwarded to the module
character by character, so `E`, `N`, `D` reached the module as message text *before* the
sketch recognised the word `END` and converted it to Ctrl+Z.

**Fix.** The sketch now buffers a **whole line** and only acts on Enter, so the helper
words never leak into the message. It also handles the two characters that "Both NL & CR"
sends per Enter, so one keypress is never read as two lines.

---

## What each fault taught

| Fault | Category | The general lesson |
|---|---|---|
| 1 · reversed capacitor | Power | Two symptoms at once usually mean one cause, not two |
| 2 · power bank cutoff | Power | A component can be working *correctly* and still break your system |
| 3 · bare `ERROR` | Method | Enable verbose diagnostics before debugging, not after |
| 4 · `>` prompt | Interface | Know how to *exit* a mode before you enter it |
| 5 · brownout | Power | Ratings on paper ≠ voltage at the pin. Measure at the load |
| 6 · sketch bug | Tooling | When debugging, suspect the test rig as well as the thing under test |

**Overall:** five of six were power or interface issues. **Zero** were faults in the
ABMDMS application code.

---

## Evidence still to capture

- [ ] Photo of the test rig — module, antenna, capacitor, supply all visible
- [ ] Photo or video of the status LED at the ~3 s rate
- [ ] Screenshot of the Serial Monitor showing `+CMGS: 184` and `OK`
- [ ] Screenshot of the received text on the phone, timestamp readable
- [ ] Note the exact room — 2G coverage varies by location

---

## Running notes

```
2026-07-30  Built the bench rig. Capacitor found reversed and dead - replaced.
2026-07-30  Power bank 5V 3A kept cutting out after the module registered.
            Diagnosed as low-current auto-cutoff. Switched to a wall adapter.
2026-07-31  Test A PASS - status LED settled to one blink every ~3 seconds.
2026-07-31  AT+CMGS returned bare ERROR. Enabled AT+CMEE=2 for real error codes.
2026-07-31  Got stuck at the ">" prompt - resolved, END sends Ctrl+Z.
2026-07-31  TEST B PASS - real SMS received on +639169751409, +CMGS: 184, OK.
2026-07-31  Module then entered a restart loop (1s blink, ~10s cycle) - brownout.
            Sends succeeding again, but the supply path has NOT been hardened yet.

(next)      Fill in the blanks above. Run Test C three times. Then Test D.

```
