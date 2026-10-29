---
name: KUDIG Database GTM
description: Split-flap departure board world for the KUDIG go-to-market page
colors:
  ground: "#0E0E0D"
  panel: "#181817"
  tile: "#1C1A14"
  tile-hi: "#232019"
  seam: "#000000"
  amber: "#F5A81C"
  amber-text: "#F7B32B"
  amber-hi: "#FFC53D"
  red: "#E23B1E"
  cream: "#E8E4DC"
  muted: "#8A8578"
  ink: "#191712"
  tile-lo: "#14120d"
  tile-foot: "#181610"
  frame-edge: "#2b2b29"
  rivet-hi: "#4a4a46"
  rivet-lo: "#151513"
  board-top: "#141413"
  board-bot: "#101010"
  ticker-bg: "#0a0a09"
  row-bg: "#121211"
  row-border: "#2c2c2a"
  chip-border: "#2c2c2a"
  panel-line: "#262624"
  sign-dash: "#b9b3a6"
  sign-num: "#6b665a"
  stat-bg: "#fffdf7"
  stat-num: "#6b665a"
  tag-cream: "#8a5a00"
  code-comment: "#6f6a5c"
  fta-lit: "#2a2113"
  fta-line: "#4a4436"
  ont-bg: "#221b0e"
  te-p2: "#d8c34a"
  footer-line: "#232322"
  glow-amber: "rgba(245,168,28,.55)"
  glow-amber-hi: "rgba(245,168,28,.8)"
  glow-amber-lo: "rgba(245,168,28,.4)"
  glow-red: "rgba(226,59,30,.7)"
  glow-red-lo: "rgba(226,59,30,.4)"
  glow-text: "rgba(247,179,43,.35)"
  wash-amber: "rgba(245,168,28,.15)"
  shadow-ink: "rgba(25,23,18,.35)"
typography:
  display:
    fontFamily: "Oswald, Noto Sans SC, sans-serif"
    fontWeight: 700
    letterSpacing: "0.02em"
  data:
    fontFamily: "Chivo Mono, Noto Sans SC, monospace"
    fontWeight: 700
    fontSize: "clamp(13px,1.5vw,26px)"
  body:
    fontFamily: "Noto Sans SC, PingFang SC, sans-serif"
    fontWeight: 400
    fontSize: "16px"
    lineHeight: 1.8
rounded:
  tile: "3px"
  btn: "6px"
  card: "10px"
spacing:
  grid-gap: "26px"
  section-y: "clamp(72px,10vw,120px)"
  z-nav: "300"
  z-skip: "400"
components:
  button-primary:
    backgroundColor: "{colors.amber}"
    textColor: "{colors.ink}"
    rounded: "{rounded.btn}"
    padding: "14px 30px"
  button-primary-hover:
    backgroundColor: "{colors.amber-hi}"
  button-ghost:
    backgroundColor: "rgba(0,0,0,.35)"
    textColor: "{colors.amber}"
    rounded: "{rounded.btn}"
    padding: "13px 22px"
  sign-card:
    backgroundColor: "{colors.cream}"
    textColor: "{colors.ink}"
    rounded: "{rounded.card}"
    padding: "18px 20px 22px"
  stat-plate:
    backgroundColor: "#fffdf7"
    textColor: "{colors.ink}"
    rounded: "6px"
    padding: "14px 16px"
---

# Design System: KUDIG Database GTM

## Overview

**Creative North Star: "The Solari Departure Board"**

The page is a large railway station: the first viewport is one wall-sized mechanical split-flap board inside a matte anodized frame, and every subsequent section is station furniture — laminated transfer signs, dual departures panels, an engineering schematic, a timetable. The metaphor is load-bearing, not decorative: each knowledge domain is a departure line, each document count is real timetable data, and the standing status is ON TIME. Density is the aesthetic: ~220 tiles fill the first viewport, glyph rhythm replaces imagery, and the machine keeps mechanically flipping one slot every 4.2 seconds so the board reads as alive.

The system is dark, warm-neutral and tactile. Amber (#F5A81C) is the single working voice — flip glyphs, the glowing CTA, sign head bands. Cream (#E8E4DC) is the physical counter-material: laminated signage the reader can touch. Red exists only as an alert register (P0 severity, system status), never as decoration.

**Key Characteristics:**
- Split-flap tile as the atomic unit: 3px radius, 2px center seam, inset shading, amber mono glyphs
- Data-first hero — the timetable rows ARE the pitch; no centered hero triad
- Monospace (Chivo Mono) reserved for data, code and measurement; never as costume
- One accent (amber), one alert (red), one paper (cream) on warm near-black
- Station vocabulary throughout: 换乘 / 线路 / ON TIME / DEPARTED / BOARDING

## Colors

One amber working voice on warm near-black, with cream signage as the physical counter-material.

### Primary
- **Board Amber** (#F5A81C): fills, CTA gradient base, sign head bands, focus accents. The voice of the machine.
- **Glyph Amber** (#F7B32B): flip-tile letterforms and ticker text — slightly lifted for legibility on tile faces.
- **Highlight Amber** (#FFC53D): hovered/active states, line-number tiles, gradient top of the CTA.

### Secondary
- **Signal Red** (#E23B1E): alert register only — P0 severity dots, red status tiles, the SYSTEM STATUS ticker segment.

### Neutral
- **Station Ground** (#0E0E0D): page field. Warm-neutral near-black; never blue-black slate.
- **Panel Charcoal** (#181817): dark panels, FTA wrap, nav — one step up from ground.
- **Tile Umber** (#1C1A14): flip-tile face; **Tile Highlight** (#232019) its upper bevel.
- **Seam Black** (#000000): the 2px tile mid-seam and all hard borders.
- **Laminated Cream** (#E8E4DC): sign cards and the For-Agents panel — paper signage.
- **Warm Gray** (#8A8578): secondary text on dark, ≥4.5:1 on ground.
- **Ink** (#191712): text on amber/cream surfaces.

### Named Rules
**The One Accent Rule.** Amber is the only accent. Red appears only where the data itself signals severity; it is never a second decorative accent.

## Typography

**Display Font:** Oswald (with Noto Sans SC)
**Body Font:** Noto Sans SC (with PingFang SC)
**Data/Mono Font:** Chivo Mono (with Noto Sans SC)

**Character:** A condensed grotesque with station-signage bloodline for display, paired with a technical monospace for every number and code token. Chinese rides Noto Sans SC at all weights; the pairing reads as one engineered voice, not two.

### Hierarchy
- **Display / Title Row** (700, clamp(26px,3.6vw,52px) per tile, tile clamps 34–66px wide): the flip-tile title KUDIG DATABASE — letters are tiles, not type.
- **Headline h2** (700, clamp(30px,4.6vw,52px), 1.15, letterspacing .02em): one per section, carries its own weight with no eyebrow above it.
- **Title h3** (700, 22px, .05em): panel and sign heads.
- **Body / Lede** (400, 16px, 1.8, max-width 640px): section leads, ≤75ch.
- **Data** (mono 700, clamp(13px,1.5vw,26px)): board rows, counts, statuses, code.
- **Label** (mono 400–500, 11–13px, letterspacing .08–.24em, uppercase where Latin): tags, nums, nav, footer.

### Named Rules
**The Measurement Voice Rule.** Monospace is for code, data and measurement. Narrative text is never set in mono.

## Layout

Full-bleed board hero: `min-height:100svh` under a fixed 56px nav; the framed board fills the remaining viewport with the title row, a one-line value tag, 8 data-row slots (`justify-content:space-evenly`) and the right-aligned CTA row; a full-width ticker closes the hero. Below, sections are a single 1180px column (`padding: clamp(72px,10vw,120px) 24px 0`): transfer signs in `auto-fit minmax(260px,1fr)` grid; the dual-read split collapses 2→1 columns at 860px; the FTA schematic and timetable are full-column. Spacing rhythm: 26px grid gaps, 4px tile gaps, field gaps of 10–26px inside board rows. Compact board rows (6-char names, no DOCS/ON TIME fields) below 760px.

## Elevation & Depth

Physical, not floating: depth comes from the board itself — inset shadow bowls (`inset 0 0 60px rgba(0,0,0,.8)`), tile bevels (inset top-light 6% / bottom-dark 60%), a 4px black frame with an outer drop (`0 20px 60px rgba(0,0,0,.6)`), and soft card lifts (`0 14px 34px rgba(0,0,0,.55)`, rising to `0 22px 44px` on hover). Zero-blur offset shadows do not exist in this world.

### Shadow Vocabulary
- **Board bowl** (`inset 0 0 60px rgba(0,0,0,.8)` + `0 20px 60px rgba(0,0,0,.6)`): the hero frame — a machine inset into the wall.
- **Card lift** (`0 14px 34px rgba(0,0,0,.55)` → `0 22px 44px rgba(0,0,0,.65)` on hover): cream sign cards lifting off the dark ground.
- **Plate shadow** (`0 4px 12px rgba(25,23,18,.35)`): stat plates inside the cream panel — soft, small, blurred.
- **CTA glow** (animated `0 0 16–34px rgba(245,168,28,.4–.8)`): the station's one luminous sign, pulsing at 2.6s.

### Named Rules
**The Machine-In-Wall Rule.** The hero board is inset into darkness; cards lift softly off it. Shadows always carry blur; nothing in this world is stamped.

## Shapes

Small radii, hard edges, mechanical repetition. Tiles 3px; buttons 6px; panels/sign cards 10px; chips/status pills 4px. Borders are black and weight-graded: 1px seams and outlines, 2px panel and card borders, 4px board frame. The rivet (12px radial-gradient circle) anchors the four corners of the board. Circular dots (8px) mark priorities and list bullets. The repeating unit is the rounded-rectangle tile with a horizontal mid-seam — any new data surface composes from it.

## Components

### Buttons
- **Shape:** 6px radius; mono 700, letterspacing .12em
- **Primary:** amber gradient (#FFC53D→#F5A81C), ink text, `14px 30px`, 1px black ring + pulsing amber glow
- **Hover / Focus:** brightness 1.08; `:focus-visible` 2px cream outline offset 3px
- **Ghost:** 1px amber border, amber text on translucent black; hover fills amber with ink text

### Chips
- **Status pills:** mono 13px, letterspacing .2em, 4px radius — DEPARTED (muted on #161615), ON TIME (amber-text on #221b0e with inner amber wash), BOARDING (ink on amber, 1.6s blink)
- **TE chips:** mono 12px, 1px #2c2c2a border on #121211, 8px priority dot (red/amber/#d8c34a)

### Cards / Containers
- **Transfer sign:** 10px radius, cream ground, 2px black border, amber gradient head band (Oswald 700 19px + 22px inline SVG train glyph), dashed #b9b3a6 list dividers, 8px amber dot bullets, hover lifts 6px
- **Dual panels:** dark (#181817, inset bowl) vs cream, 2px borders, 28px padding, mono audience tags (FOR HUMANS / FOR AGENTS)

### Inputs / Fields
- Not present on this surface; the code plate (`pre`, dark #191712, amber-text, 1px border, 6px radius, `overflow-x:auto`) is the closest grammar.

### Navigation
- Fixed 56px, rgba(24,24,23,.92) + 6px backdrop blur; mono wordmark KUDIG-**IO** (amber), 12px uppercase letterspaced links (muted → amber-hi on hover/focus), GitHub ★ as 1px amber outlined pill; links below 760px collapse to wordmark + pill.

### Split-Flap Tile (signature)
The atomic component: span tile, 3px radius, four-stop vertical face gradient (#232019→#1C1A14→#14120d→#181610), 2px black mid-seam at 50% with a 1px top-light, inset bevels + 2–3px drop, amber mono glyph with a soft amber glow. Entrance: `rotateX(88deg)→0` with brightness flash, 6ms per-tile stagger; idle: one slot re-flips every 4.2s; `prefers-reduced-motion` freezes everything.

## Do's and Don'ts

### Do:
- **Do** compose any new data surface from split-flap tiles (3px radius, mid-seam, amber mono glyphs).
- **Do** keep every number real and traceable to the repo — scale IS the pitch.
- **Do** use station vocabulary (线路 / 换乘 / ON TIME / DEPARTED / BOARDING) for states and sections.
- **Do** theme browser surfaces: amber `::selection` on ink, SVG data-URI tile favicon, `theme-color` #0E0E0D.
- **Do** ship a full `prefers-reduced-motion` downgrade (static tiles, single-pass ticker).

### Don't:
- **Don't** put a kicker or eyebrow label above a heading — the heading carries its own weight.
- **Don't** use zero-blur offset shadows — this world is inset and lifting, never stamped.
- **Don't** introduce a second accent color; red is reserved for severity data.
- **Don't** set narrative text in monospace, or use a system display face as the display voice.
- **Don't** drift the ground toward blue-black slate or warm cream backgrounds — the field stays #0E0E0D.
- **Don't** invent metrics, testimonials or customer logos; unprovable claims stay off the board.
