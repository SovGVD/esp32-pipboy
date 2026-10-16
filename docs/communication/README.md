# Emoji / emote communication

[Project overview](../../README.md) · [Screens](../screens/README.md) · [Music](../music/README.md)

RADIO → EMOTES exchanges twelve named emotes with nearby Pip-Boys using
ESP-NOW. Emotes currently appear as text notifications; emoji artwork and
animated emote messages are not implemented. A Wi-Fi router or internet
connection is not required.

## Sending and receiving

1. Power on two compatible Pip-Boys within radio range. Give them distinct
   device names: discovery and recipient matching use those names.
2. Turn the main encoder to RADIO and the secondary encoder to EMOTES. If you
   are browsing MUSIC, hold the secondary button to return to subpage selection.
3. Allow a few seconds for discovery. Click to browse the peer list, turn to
   choose a peer, and click to select it.
4. Turn to choose an emote and click to send. The screen returns to the peer
   list after sending. Hold to return to subpage selection.

The recipient sees a message window with the sender's name and emote text.
Click or hold to dismiss it. A running game pauses; click again to resume
after dismissing the message. Background music continues.

**NO SIGNAL** means that no online peer is listed. Check that both devices
run compatible firmware and use the same radio channel. There is no delivery
receipt or message history in the UI.

## Available emotes

| Display text | Protocol code |
| --- | --- |
| Confused | `confused` |
| Finger Guns | `fingergu` |
| Follow! | `follow` |
| Frown | `frown` |
| Heart | `heart` |
| Join My Team! | `joinmyte` |
| Radiated | `radiated` |
| Startled | `startled` |
| Thumbs Down | `thdown` |
| Thumbs Up | `thup` |
| Wave | `wave` |
| Fear | `fear` |

## Discovery and message format

[src/appRadio.cpp](../../src/appRadio.cpp) initializes QuickESPNow on channel 1.
Devices broadcast a discovery ping roughly every three seconds. Up to four
peers are tracked, with entries expiring after 150 UI ticks (about five seconds
at 30 frames per second). Names fit in 15 characters and are stored in EEPROM.

Packets have five tab-separated fields:

```text
PIPBOY<TAB>sender<TAB>recipient<TAB>command<TAB>body
```

`<TAB>` represents a literal tab byte. Discovery uses recipient `_`, command
`P` and body `_`. An emote uses the selected recipient's name, command `E` and
one of the codes above. Frames are broadcast at the ESP-NOW transport level;
the receiver filters emotes by recipient name, with `_` accepted as broadcast.
This implementation has no authentication or encryption.

The receive callback accepts at most 126 bytes with no embedded NUL and copies
packets into a four-entry queue. Parsing, peer updates and notifications run
on the UI thread. Excess packets can be dropped when the queue is full.

## Checks on two devices

- Confirm discovery in both directions, then send each emote and dismiss it.
- Move one device out of range or turn it off; confirm it leaves the peer list.
- Receive an emote while playing music and while a game is running; check
  uninterrupted audio and game pause/dismiss/resume behavior.

These checks require two physical devices. Host tests of notification controls
do not establish ESP-NOW delivery or radio range.
