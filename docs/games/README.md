# DATA arcade

[Project overview](../../README.md) · [Screens](../screens/README.md) · [Music](../music/README.md)

DATA → GAMES has two small original games with monochrome vector/pixel art:

| Game | Style | Goal |
| --- | --- | --- |
| ATOM BREAKER | Arkanoid-style brick-breaker | Return the ball with the paddle and clear 32 bricks. |
| STAR PATROL | Galaga-style space shooter | Dodge enemy bullets and diving attackers while clearing an 18-ship formation. |

## Controls

1. Turn the main encoder to DATA. Click the secondary encoder to enter the
   GAMES list; turn it to select a game and click again to open it.
2. Turn the secondary encoder to move the paddle or player ship. All detents
   between frames are consumed, so quick turns move farther than a single step.
3. Click to launch the ball/start the wave. During play, click pauses; click
   again resumes. STAR PATROL fires automatically, so no held fire button is
   needed while turning the dial.
4. Hold the secondary button to return to the game list. Reopening the same
   game preserves its board, score and lives; click to resume if it was playing.
   Hold again from the game list to return to category navigation.
5. Touch works too: tap a game twice to open it; hold a finger on the bottom
   row (paddle/ship) and it moves toward the finger; tap anywhere above that
   row to launch, pause or resume; tap GAMES to return to the list.
6. The main encoder always controls the top-level tabs. Leaving DATA pauses the
   open game. On return, click to resume; time spent elsewhere is not simulated.

ESP-NOW message windows pause games and receive the dismiss click/hold without
also launching, unpausing or exiting a game. Dismiss the window, then click to
resume. Radio music remains on its independent task. Games generate no sound
effects and do not take over the audio output.

## Rules and state

Both games start with three lives. Missing the ball costs a life in ATOM
BREAKER; a hit from a bullet or diving enemy costs a life in STAR PATROL. The
shooter briefly blinks the ship during a 1.4-second hit grace period.

Brick hits score 10–40 points by row. Shooter kills score 25–75 points by row,
with a 50-point bonus for a diving attacker. Clearing a board/wave waits for a
click before preparing the next wave, then another click launches it. Speeds
and attack rates increase through wave 10 and remain bounded thereafter. The
wave display caps at 999 and scores saturate at the 32-bit unsigned maximum.

At GAME OVER, click to prepare a fresh run and click again to start. The launcher
shows the selected game's best score. Each game retains its own run and best
score while powered on; these are not written to EEPROM or SD and reset on boot.

## Implementation

- `lib/Arcade/src/Arcade.cpp` is the deterministic simulation, with bounded
  fixed arrays for bricks, enemies and projectiles. It has no Arduino, display,
  audio, filesystem or allocation dependency.
- Simulation uses 8 ms substeps, with at most 96 ms of catch-up per UI frame.
  Pause/launch clears leftover time so tab changes, notifications and slow
  frames cannot cause an unbounded update or an instant life loss on resume.
- `ArcadeRender.cpp` draws the launcher, playfield, HUD and phase overlays at
  480×272 using the bundled monofonto10 font and the existing green palette.
- `src/appData.cpp` owns both sessions and adapts their drawing to Arduino_GFX.
  `src/encoders.cpp` routes game input before the normal submenu/list handlers.
  The shared UI font is restored after every DATA render.

## Automated checks and previews

```sh
./test/host/run.sh
pio run
# Optional software previews; requires host g++ and Pillow:
python3 scripts/preview_games.py --output docs/games/previews
```

The sanitizer-backed checks cover paddle/brick/wall/corner contacts, score and
life changes, wave clear/restart, enemy shots/dives and hit grace, deterministic
time partitioning, catch-up limits, paused time, and 100,000 seeded updates.
Native render checks cover both launchers, every phase, maximum score/wave text
and long play sequences. Control tests compile the actual DATA and encoder
sources against host doubles and check all detents, pause/resume, leaving and
returning to DATA, message dismissal, hold-to-back and font restoration. Existing
inventory, ID3 and background-audio checks remain in the same test command.

[All game screens](previews/contact-sheet.png) ·
[ATOM BREAKER](previews/breaker-play.png) ·
[STAR PATROL](previews/patrol-play.png)

Images use the actual firmware simulation, layout and bundled font glyphs with
a host rasterizer. They are reference renders, not captures from the board.
Some phase previews explicitly select the overlay being reviewed.

## Device acceptance

After flashing, play both games with slow and fast encoder turns. Check paddle
and ship travel, click and hold behavior, legibility, frame pacing and difficulty.
Play music, leave DATA, return, and receive an ESP-NOW message mid-game; confirm
that the game pauses predictably and the music remains uninterrupted. Automated
tests do not establish the physical control feel or audible/device acceptance.
