# Checkmate

![Checkmate in play](https://github.com/user-attachments/assets/2f015a95-a2ec-4de4-91b0-2b8e0893f70c)

A native 3D chess game for Omarchy, built with C++ and Qt Quick 3D. Play against a friend on the same device or against the built-in computer, with four original sculptural piece sets: Orbital Orders, Midnight Atelier, Moss & Myth, and Candy Kingdom.

The sets have individually sculpted pieces: swept metal panels, orbital rings and a crescent queen; carved wood and ivory horses; woodland foxes, owls, acorn sprites and antlered royalty; and glossy candy pieces with curled manes and gold crowns.

Every set moves in its own manner. Orbital craft hover over their squares with pulsing lights, bank into a glide and scan when chosen. Midnight Atelier's pieces are handled under flickering candlelight: tilted in unseen fingers, set down with a rock, and tipped over when taken. Moss & Myth's creatures breathe and stir, turn to watch a neighbour being picked, and leap. Candy Kingdom's sweets wobble like jelly and pop. Knights jump highest everywhere, and check gets a brief pulse. Reduced motion is available in Settings. The interface follows Omarchy's theme and language; the board's materials follow the selected set.

## Play

Click a piece, then a highlighted square. Right-drag to orbit the board and scroll to zoom. Choose New game for local two-player play, a computer opponent, your color, and three difficulty levels. Games have no clock and save after each move. Undo takes back both sides' moves against the computer.

The rules include castling, en passant, all four promotion choices, checkmate, stalemate, and insufficient material. Threefold repetition and the fifty-move rule end a game automatically. Computer difficulty names describe search effort, not an Elo rating.

| Key | Action |
| --- | --- |
| Arrow keys | Navigate squares |
| Space / Enter | Select or move |
| Escape | Cancel selection |
| Ctrl+Z | Undo |
| N | New game |
| F | Flip board |
| R | Reset camera |
| 1–4 | Switch piece set |
| Tab | Show or hide the interface |
| S | Settings |
| ? | Show shortcuts |
| Q | Quit |

## Build

Dependencies: CMake, a C++17 compiler, Qt 6 base, declarative, Quick 3D, SVG, and Wayland. `bin/build` installs missing build and test dependencies automatically on Arch/Omarchy, asking for your sudo password if needed. Subsequent builds skip installation when everything is present. Other distributions need these dependencies installed through their package manager first.

```bash
bin/build
bin/test
build/checkmate open
cmake --install build --prefix ~/.local
```

The tests use a virtual display with the offscreen platform and Mesa software OpenGL, and fail on Qt warnings so a blank 3D viewport cannot pass unnoticed.

The package recipe belongs in omarchy-pkgs. CI reads dependencies from that recipe once it lands; until then `bin/test` is the local check. Tests exercise the real QML window, theme and language watchers, chess move trees, special moves, draw detection, undo, save/restore, and computer cancellation.

## Headless commands

These commands do not need a display and do not change the saved game:

```bash
checkmate inspect --json
checkmate export > game.pgn
checkmate export --json
checkmate legal --json
checkmate legal 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1' --json
```

`inspect` reports FEN, moves, and the result; `export` prints PGN; `legal` lists coordinate moves for a FEN or the starting position. Errors go to stderr with exit status 1. Successful commands exit 0. A second graphical launch brings the existing game forward instead of opening a competing writer.

English, Danish, and Arabic catalogs are included. Other Omarchy languages currently fall back to English. The board keeps standard chess coordinates when the surrounding interface mirrors for right-to-left reading.

All 3D pieces are generated from original geometry in the binary. No downloaded models, franchise characters, network connection, or separate chess engine is needed. The game is saved atomically in the standard application data directory as `checkmate/game.json`.

MIT license. Shared theme, translation, and build infrastructure follows the Omarchy app templates by David Heinemeier Hansson.
