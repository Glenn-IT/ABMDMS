# SMS Module Test — Results Log

**Module:** SIM800L V2.2 (UNV), 5 V board
**Board:** Arduino Uno
**Recipient tested:** `+639169751409`
**Date tested:** 30 July – 1 August 2026
**Tested by:** Glenn
**Room / location:** ____________ *(fill in — 2G coverage is location-specific)*
**Supply:** 5 V 2 A wall adapter · **Capacitor:** 1000 µF 16 V *(second one — the first failed, Fault 7)*

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

**Power stability solved on 1 August 2026.** A failed capacitor was causing the module to
restart after every transmit (Fault 7). With it replaced, the integrated `pir_sms_test`
rig ran **10 minutes continuously with no restart**, detecting motion and sending texts
throughout — the first sustained clean run of the whole chain.

---

## Test status

| Test | What it proves | Status |
|---|---|---|
| Pre-check | SIM works on 2G in a normal phone | ☐ not formally done — proven indirectly by the successful send |
| **A** — power + status LED | Module alive, joined the network | ✅ **PASS** — settled to one blink every ~3 s |
| **B** — manual AT commands | Arduino can drive it; it can send | ✅ **PASS** — real text received, `+CMGS: 184` |
| **C** — automatic self-test | Same, hands-free, repeatable | ⬜ **NOT YET RUN** — superseded in practice by the `pir_sms_test` rig |
| **D** — battery / laptop off | Alerts work with no computer | ⬜ **NOT YET RUN** — blocked on the power-bank cutoff |
| **Integrated** — PIR triggers a real text | The whole chain, unattended | ✅ **PASS** — 1 Aug 2026, 10 min clean run on `pir_sms_test` |

---

## Still to confirm (blanks that matter)

These were never written down and should be, because they decide whether the build is
reliable or merely lucky:

| Item | Value |
|---|---|
| Wall adapter **amp rating** | ✅ **5 V 2 A** — meets the minimum |
| Network (Globe / TM / Smart / TNT) | ____________ |
| `AT+CSQ` reading | `+CSQ: ______` *(want above 10 — the sketch now prints this at start-up)* |
| `AT+CREG?` reading | ____________ *(want `0,1` or `0,5`)* |
| `AT+CSCA?` reading | ____________ |
| Capacitor **voltage rating** fitted | ✅ **16 V** — comfortably above the 10 V minimum |
| Capacitance fitted | ✅ **1000 µF** |
| Is module power still routed through the breadboard? | ☐ yes ☐ no — **should be no** |

The `AT+CSQ` blank is now easy to fill: since `ATE0` was added, the sketch prints a real
signal number on every start-up instead of the echo of its own question.

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

**Status: it came back, exactly as predicted below. See Fault 7 — the cause was a failed
capacitor.**

The original note read: *"resolved in practice, but not proven fixed — if it recovered
because conditions were briefly favourable rather than because the supply path changed,
it will come back."* It came back on the integrated rig two days later. Writing that
prediction down is what made the second occurrence quick to recognise.

**Still worth doing before the demo.** Run `5Vin` and `GND` as a **short, thick, direct
pair from the adapter to the module**, bypassing the breadboard. Breadboard spring
contacts are rated ~1 A each and dupont wire is thin — at 2 A the voltage is lost before
it reaches the module. Capacitor twisted or soldered onto the module's own terminals, and
more bulk capacitance (2200 µF, or a second 1000 µF in parallel) costs nothing and buys
headroom. Fault 7 was fixed by replacing the capacitor, not by hardening the path, so
this margin has still never been added.

---

## Fault 7 — module restarted after every send on the integrated rig

**Date: 1 August 2026. Found on `pir_sms_test`, the 1-PIR + SIM800L rig.**

**Symptom.** The first text sent successfully. After that, every triggered send killed the
module: LED dark, then a fast ~1 s blink for about 10 seconds, then settling to one blink
every ~3 seconds — and the next wave of the hand repeated the whole cycle. In the serial
log it appeared as a **`TIMEOUT` immediately followed by an `ERROR`**, twice over:

```
SMS_SENT:ROOM1             <- the one that worked
...
SMS_FAIL:ROOM1:TIMEOUT     <- module died mid-send, never returned +CMGS
SMS_FAIL:ROOM1:ERROR       <- it rebooted; a reboot forgets AT+CMGF=1
SIM_RESET                  <- three fails in a row triggered the sketch's own recovery
```

**Cause.** **The 1000 µF capacitor had failed.** Not reversed this time — correct value,
correct 16 V rating, correct polarity, physically fitted. It had simply stopped working.

**Why it took two days.** Every visible fact pointed away from the capacitor. The supply
was a 5 V 2 A wall adapter, so Fault 2's power-bank cutoff was ruled out. The capacitor
was the right part, fitted the right way round, and looked fine. Suspicion went to the
supply path, the adapter, and the sketch before it came back to the component that was
sitting there apparently doing its job.

**How it was found.** Testing the capacitor directly rather than trusting its appearance.
The full procedure is now written up in `POWER_TROUBLESHOOTING.md` — visual check,
capacitance mode, resistance-climb test, and the remove-it isolation test.

**Fix.** Replaced the capacitor. The rig then ran **10 minutes with no restart at all**,
detecting motion and sending texts throughout.

**Raw evidence.** The captured serial log is kept at `pir_sms_test/Issues.md`.

**Lesson.** *Present, correct and correctly fitted* is not the same as *functional*. A
capacitor is the cheapest and fastest thing in the whole chain to test, so it should be
the first thing tested, not the fourth. This is the second time on this build that the
capacitor was the answer (see Fault 1) and the second time it was suspected late.

---

## Fault 8 — four robustness bugs in the sketch, exposed by Fault 7 *(software)*

The brownout did not cause these, but it made them visible: once one send failed, the
sketch turned a single failure into a cascade instead of recovering.

| Bug | Effect | Fix |
|---|---|---|
| Command **echo never turned off** | The module repeats each command back, so `AT+CSQ` matched as its own answer. The start-up line printed `Signal: )-5?AT+CSQ` — the question, not the reading | `ATE0` during start-up |
| **No ESC on failure** | A timed-out send left the module sitting at its `>` prompt, where it swallows the next `AT+CMGS` — so one failure bred the next | Send ESC (27) on any failure to abandon the half-typed message |
| **No rest between sends** | A retry fired the instant the previous one failed, before the module had finished tidying up | Restored a 5 s `SMS_MIN_GAP_MS` |
| **`simReset()` lost its settings** | A reset forgets `ATE0` and `AT+CMGF=1`; the recovered module was left in the wrong mode | Re-issue both after every reset |

`AT+CMEE=2` was also added so failures report a real `+CMS ERROR` code rather than a bare
`ERROR` — the same lesson as Fault 3, now applied to the production sketch instead of only
the bench tools.

**Lesson.** These were all *recovery* bugs. They cost nothing while the hardware behaved
and cost a great deal the moment it did not. Error paths deserve the same attention as the
happy path, and the only way to find out whether they work is to make something fail.

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
| 7 · failed capacitor | Power | *Present and correct* ≠ *functional*. Test the cheap part first |
| 8 · recovery bugs | Software | Error paths only get tested when something actually fails |

**Overall:** six of eight were power or interface issues. The two software faults were both
in **recovery paths**, not in the application logic — no motion event was ever mis-recorded,
no bad data ever reached the database.

**The pattern across all eight:** the fault was almost never where the symptom pointed.
A dying capacitor presented as a network problem, a working power bank presented as a
broken module, and a reboot presented as a text-mode error. The habit that shortened each
hunt was writing down what was *observed* separately from what it was *assumed to mean*.

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
2026-08-01  Built pir_sms_test - 1 PIR + SIM800L, own database and dashboard.
            PIR half worked first try. First SMS sent successfully.
2026-08-01  Then the restart loop came back on every send. Serial showed
            TIMEOUT followed immediately by ERROR, repeating. Wall adapter
            5V 2A, so the power-bank cutoff (Fault 2) was ruled out.
2026-08-01  Fixed four recovery bugs in the sketch: ATE0 (echo was on and
            was corrupting every reply), ESC on failure, 5s gap between
            sends, re-init after reset. Recovers cleanly now, but the
            restarts continued - so the hardware was still the real fault.
2026-08-01  FOUND IT: the 1000uF capacitor had failed. Right value, right
            16V rating, right polarity, looked perfect - simply dead.
            Two days lost because it was the last thing suspected.
2026-08-01  Capacitor replaced. RIG RAN 10 MINUTES WITH NO RESTART.
            Motion detected and texts sending throughout. Fault 7 closed.

(next)      Harden the supply path anyway - 5Vin/GND direct, off the
            breadboard - so the margin exists before the demo.
            Fill in the CSQ / CREG / network blanks above.
            Run Test C three times. Then Test D on battery.

```
