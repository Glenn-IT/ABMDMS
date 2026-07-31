# AT Command Reference — SIM800L bench test

For **Test B** (`sim800l_passthrough/sim800l_passthrough.ino`).
Serial Monitor at **9600**, line ending **"Both NL & CR"**.

> If the line ending is wrong, the module ignores everything you type and
> looks dead. Check that dropdown first, every time.

---

## The sequence

Type these one at a time and wait for each answer.

| # | You type | Good answer | What it proves |
|---|---|---|---|
| 1 | `AT` | `OK` | Powered, wired right, talking |
| 2 | `ATE0` | `OK` | Turns off echo (optional, makes replies cleaner) |
| 3 | `AT+CPIN?` | `+CPIN: READY` | SIM seated, PIN lock off |
| 4 | `AT+CSQ` | `+CSQ: 17,0` | Signal — first number **must be above 10**, `99` = none |
| 5 | `AT+CREG?` | `+CREG: 0,1` | Network accepted the SIM (`0,5` = roaming, also fine) |
| 6 | `AT+COPS?` | `+COPS: 0,0,"GLOBE"` | Which network it joined (nice for your report) |
| 7 | `AT+CMGF=1` | `OK` | Plain-text SMS mode |
| 8 | `AT+CMGS="+639171234567"` | `>` | Ready for the message body |
| 9 | *type your message* | *(nothing yet)* | |
| 10 | `END` then Enter | `+CMGS: 42` | **Sent** — the number is just a message counter |

Step 10: the passthrough sketch turns the word `END` into the **Ctrl+Z**
character for you, because the Arduino IDE's Serial Monitor cannot type it.

### Sending a second message — you do not repeat the whole sequence

`AT+CMEE=2` and `AT+CMGF=1` live in the module's RAM and **persist for as long as it stays
powered and does not reset**. Set once per session, not once per message. To send again,
just:

```
AT+CMGS="+639169751409"
your message
END
```

- `AT+CMEE=2` — diagnostics only, never required to send
- `AT+CMGF=1` — **required**, and wiped by any reset

Check the mode any time instead of guessing:

| | |
|---|---|
| `AT+CMGF?` → `+CMGF: 1` | still in text mode, go ahead |
| `AT+CMGF?` → `+CMGF: 0` | it reset — run `AT+CMGF=1` first |

Make the setting survive a reboot:

```
AT&W        saves the current settings to the module's stored profile
```

Worth doing on a build that has had brownouts — it means a reset costs you nothing.

> This is exactly why `motion_sensor.ino` calls `AT+CMGF=1` inside `simSetup()` on every
> boot: the Arduino cannot assume the module remembers anything.

`+CMGS: <n>` is a running counter of messages sent, not a status. The `OK` after it is
what confirms success.

### The `>` prompt is a mode — know how to get out of it

Once `>` appears the module has stopped listening for commands. **Everything you type
is message text**, including AT commands typed by mistake. Pressing Enter does not send
anything, it just adds another line to the message.

| Type this | What happens |
|---|---|
| `END` | Sends Ctrl+Z — the message goes out. Wait 5–30 s for `+CMGS: <n>` |
| `CANCEL` | Sends ESC — throws the message away, back to normal AT commands |
| `RESET` | Hardware-resets the module (pulses Pin 12). Loses `CMGF`, so set `AT+CMGF=1` again |

If you have been sitting at `>` for a while unsure what went into the message, `CANCEL`
and start the `AT+CMGS` line again. A message never sent costs nothing.

---

## When you get a bare `ERROR` — read this first

`ERROR` on its own tells you nothing. Turn on verbose errors and the module will
name the fault instead:

```
AT+CMEE=2
```

Now every failure comes back as `+CMS ERROR: <code>` or `+CME ERROR: <text>`.

| Code | Meaning | Fix |
|---|---|---|
| `305` | Invalid text mode parameter | You are in PDU mode — `AT+CMGF=1` was lost. **Usually means the module reset**, which means a brownout |
| `310` | SIM not inserted | Reseat the SIM |
| `311` / `312` | SIM PIN required | Turn the PIN lock off in a phone |
| `321` | Invalid memory index | Harmless here |
| `330` | **SMSC address unknown** | `AT+CSCA?` is empty — set it, see below |
| `331` / `332` | No network service / timeout | Coverage. Check `AT+CSQ` and `AT+CREG?` |
| `500` | Unknown error | Nearly always **no load / credit** on the SIM |
| `512` | Operator barring / SIM problem | SIM expired, barred, or not provisioned for SMS |
| `21` / `38` | Network rejected the message | Wrong number format, or an operator block |

### The two failures that look identical

**A bare `ERROR` straight after `AT+CMGS`, with no `>` prompt at all**, when the same
command gave you a `>` a minute earlier, means `CMGF` is no longer 1. Nothing you typed
changed it — so **the module restarted**. Go to `POWER_TROUBLESHOOTING.md`, Symptom 3.

**`ERROR` after you send the body with Ctrl+Z** means the message was rejected rather
than never attempted: no credit, no SMSC, or the network refused it.

### Checking the SMSC

The service centre is the operator number your SIM hands messages to. If it is blank,
every send fails with `ERROR` even when signal, registration and credit are all perfect.

```
AT+CSCA?
```

Good answer: `+CSCA: "+639170000130",145` — a real number.
Bad answer: empty, or all zeros. Set it by hand:

| Network | Command |
|---|---|
| Globe / TM | `AT+CSCA="+639170000130"` |
| Smart / TNT | `AT+CSCA="+639180000101"` |

> **DITO will never work.** DITO has no 2G network, and the SIM800L is 2G only.
> Use a Globe, TM, Smart or TNT SIM.

### Checking the credit through the module

```
AT+CUSD=1,"*143#"     Globe
AT+CUSD=1,"*123#"     Smart
```

The operator's reply comes back as text in the Serial Monitor.

---

## Extra commands worth knowing

| Command | Answer | Use |
|---|---|---|
| `AT+CBC` | `+CBC: 0,85,4050` | Supply voltage in mV — the last number. Below ~3600 while sending means your power is too weak |
| `AT+GSV` | model / firmware | Confirms it really is a SIM800L |
| `AT+CCID` | long number | Reads the SIM's serial — proves the SIM holder contacts are good |
| `AT+CUSD=1,"*143#"` | operator menu | Checks load/credit through the module itself (Globe: `*143#`, Smart: `*123#`) |
| `AT+CMGL="ALL"` | list | Reads messages *received* by the SIM |
| `AT&F` | `OK` | Factory reset the module's settings |

---

## Decoding the answers

### `AT+CSQ` — signal quality

| First number | Meaning |
|---|---|
| `0–9` | Too weak. Sends will fail randomly |
| `10–14` | Usable, marginal |
| `15–19` | Good |
| `20–31` | Strong |
| `99` | **No signal detected at all** — antenna missing, or no 2G here |

### `AT+CREG?` — network registration

| Answer | Meaning | What to do |
|---|---|---|
| `0,1` | Registered, home network | ✅ carry on |
| `0,5` | Registered, roaming | ✅ carry on |
| `0,2` | Searching | Wait 30 s. Still searching = no 2G coverage |
| `0,3` | **Denied** | SIM barred, expired, or no load |
| `0,0` | Not searching at all | Module problem — reset it |

### `AT+CPIN?`

| Answer | Meaning |
|---|---|
| `+CPIN: READY` | ✅ good |
| `+CPIN: SIM PIN` | PIN lock is still on — turn it off in a phone |
| `ERROR` | No SIM detected — reseat it, check it is 2FF and the right way round |

---

## Fix table

| What you see | Usual cause | Fix |
|---|---|---|
| Nothing at all from `AT` | No common ground | Join module GND, supply GND and Arduino GND at one point |
| Nothing at all from `AT` | TX/RX swapped | Module `TXD`→Pin 10, module `RXD`→Pin 11. They cross |
| Nothing at all from `AT` | Powered from the Arduino 5V pin | Must be an external supply — the Uno cannot give 2 A |
| Nothing at all from `AT` | Wrong line ending | Set the Serial Monitor to "Both NL & CR" |
| Garbage characters | Baud mismatch or 2.8 V logic levels | Confirm 9600 both ends; on a bare module use a level converter |
| Status LED blinks once a second, forever | Not registered | 2G coverage / antenna / SIM. Retest the SIM in a 2G-forced phone |
| Module reboots when you press Ctrl+Z | Brownout on the transmit burst | The 1000 µF cap must be **across** 5Vin/GND, and the supply must hold up |
| `AT+CMGS` → `ERROR` | No load, or the number is not international format | Top up; write the number as `+639XXXXXXXXX` |
| Everything passes, no text arrives | Wrong recipient number | Check digit by digit, including the `+63` |
| Random failures over time | SoftwareSerial dropping bytes | An Arduino Mega's real second port (`Serial1`) removes this entirely |

---

## What a full, successful Test B looks like

```
AT
OK

AT+CPIN?
+CPIN: READY
OK

AT+CSQ
+CSQ: 17,0
OK

AT+CREG?
+CREG: 0,1
OK

AT+CMGF=1
OK

AT+CMGS="+639171234567"
> ABMDMS test
END
[sent Ctrl+Z - waiting for +CMGS: ...]
+CMGS: 42
OK
```

Then the phone buzzes. That is the whole goal of this folder — copy those
numbers into `RESULTS.md`.
