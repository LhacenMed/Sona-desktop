# Sona

A native Windows media player (C++20, Win32, Direct2D/DirectComposition, libVLC 3) built to the Figma
"VLC Redesign".

Before working, read:

1. `docs/ROADMAP.md`: the full UI specification, the system pieces still to build, the phased plan and
   the open decisions. Continue from the phase it marks as next.
2. `docs/ARCHITECTURE.md`: the modules and the rules the code follows.
3. The Figma export in `design/` (gitignored): the PNG of the frame you're building, and
   `design/figma.json` for its exact numbers. Don't use the Figma MCP for this file.

Work on the `dev` branch. Build and verify commands are in `docs/ROADMAP.md` section 9.
