# RFC-001: Per-Sprite Collision Hitbox Margins

**Status:** Proposed
**Target:** `msxbas2rom`
**Scope:** BASIC language, compiler and runtime kernel
**Related feature:** `COLLISION()`

---

## 1. Summary

This RFC proposes support for **per-sprite collision hitbox margins** in `msxbas2rom`.

The current collision system treats each sprite as a full rectangular area. This is simple and fast, but it can produce collisions between transparent or visually irrelevant portions of sprites.

The proposed feature allows each sprite to define four margins:

* `LEFT`
* `TOP`
* `RIGHT`
* `BOTTOM`

These margins reduce the effective collision rectangle without changing the sprite's visual representation.

The key design principle is:

> **Hitbox configuration is resolved outside the collision hot path as much as practical. `COLLISION()` consumes pre-normalized relative hitbox bounds and combines them with the current sprite position.**

This allows the existing collision algorithm to remain structurally similar while keeping hitbox configuration persistent across sprite movement.

---

# 2. Motivation

A sprite is normally represented by a rectangular area, but its visible shape is often smaller or irregular.

For example:

```text
+----------------+
|                |
|     ██████     |
|   ██████████   |
|   ██████████   |
|     ██████     |
|                |
+----------------+
```

The current collision rectangle includes the entire 16×16 area.

This may cause a collision even when the visible portions of two sprites do not actually overlap.

Hitbox margins allow the collision area to better approximate the visible object:

```text
+----------------+
|                |
|   +--------+   |
|   | ██████ |   |
|   |████████|   |
|   |████████|   |
|   +--------+   |
|                |
+----------------+
```

The feature also provides an automatic mode capable of deriving the collision area from the sprite pattern currently stored in VRAM.

---

# 3. Goals

This RFC has the following goals:

1. Allow collision hitboxes to be configured independently for each sprite.
2. Provide explicit manual hitbox configuration.
3. Provide an automatic hitbox calculation based on the sprite pattern.
4. Make hitbox configuration persistent until explicitly changed.
5. Keep `COLLISION()`'s public API unchanged.
6. Keep the existing `SPRTBL` layout unchanged.
7. Avoid VRAM access during collision detection.
8. Avoid a global hitbox-mode check inside `COLLISION()`.
9. Minimize additional work in the collision hot path.
10. Preserve the behavior of existing programs when hitboxes are not configured.

---

# 4. Non-Goals

This RFC does not introduce:

* pixel-perfect collision detection;
* rotated or arbitrarily shaped hitboxes;
* per-pixel collision masks;
* dynamic automatic hitbox recalculation;
* changes to the `COLLISION()` API;
* changes to the visual appearance of sprites;
* changes to the existing sprite table layout.

The `AUTO` operation is explicitly a **one-shot calculation**, not a continuously updated collision mechanism.

---

# 5. Existing Collision Model

The current runtime maintains the following structure for each sprite:

```text
SPRTBL + 5 × sprite

+0  active
+1  x0
+2  x1
+3  y0
+4  y1
```

The existing collision test is an axis-aligned bounding-box (AABB) test:

```text
bx0 < ax1
ax0 < bx1
by0 < ay1
ay0 < by1
```

All four conditions must be true for a collision.

The existing rule that two sprites occupying exactly the same X/Y position are considered the same object remains unchanged.

---

# 6. Proposed BASIC Syntax

The syntax intentionally follows the existing `SET SPRITE ...` family used by
sprite extended commands. This keeps the language surface consistent and makes
the parser/compiler integration closer to the current `SET SPRITE COLOR`,
`SET SPRITE PATTERN`, `SET SPRITE FLIP`, and `SET SPRITE ROTATE` implementation.

## 6.1 Enable default hitboxes

```basic
SET SPRITE HITBOX ON
```

This initializes all 32 sprites with zero margins:

```text
LEFT   = 0
TOP    = 0
RIGHT  = 0
BOTTOM = 0
```

This is also the **runtime default state**.

Therefore, a program that does not execute any `SET SPRITE HITBOX` command behaves exactly as it does today.

---

## 6.2 Disable collision hitboxes

```basic
SET SPRITE HITBOX OFF
```

This disables collision for all 32 sprites by clearing each sprite's hitbox
enabled flag (see section 9). The relative bounds are left as the default full
rectangle, but the sprite no longer participates in collision detection.

The command is therefore a convenient way to make all sprites non-collidable
without introducing a separate global collision-mode flag.

---

## 6.3 Configure a single sprite

```basic
SET SPRITE HITBOX <sprite>
```

sets all margins for the specified sprite to zero.

For example:

```basic
SET SPRITE HITBOX 3
```

is equivalent to:

```text
sprite 3:
LEFT   = 0
TOP    = 0
RIGHT  = 0
BOTTOM = 0
```

---

## 6.4 Configure a single sprite with ON/OFF/AUTO

In addition to the bare form, a specific sprite can be enabled, disabled, or
auto-configured individually:

```basic
SET SPRITE HITBOX <sprite> ON
SET SPRITE HITBOX <sprite> OFF
SET SPRITE HITBOX <sprite> AUTO
```

`SET SPRITE HITBOX <sprite> ON` is equivalent to `SET SPRITE HITBOX <sprite>`:
it resets the sprite's margins to zero and enables collision.

For example:

```basic
SET SPRITE HITBOX 3 ON
```

produces:

```text
sprite 3:
LEFT   = 0
TOP    = 0
RIGHT  = 0
BOTTOM = 0
enabled
```

`SET SPRITE HITBOX <sprite> OFF` disables collision for that sprite only by
clearing its enabled flag; its margins are left unchanged.

`SET SPRITE HITBOX <sprite> AUTO` derives that sprite's hitbox from its VRAM
pattern (a one-shot calculation, as described in section 15); a fully
transparent sprite is disabled.

---

## 6.5 Configure explicit margins

```basic
SET SPRITE HITBOX <sprite>, <left>, <top>, <right>, <bottom>
```

Example:

```basic
SET SPRITE HITBOX 3, 2, 3, 2, 4
```

defines:

```text
LEFT   = 2
TOP    = 3
RIGHT  = 2
BOTTOM = 4
```

Omitted trailing parameters default to zero.

Therefore:

```basic
SET SPRITE HITBOX 3, 2
```

means:

```text
LEFT   = 2
TOP    = 0
RIGHT  = 0
BOTTOM = 0
```

---

# 7. Margin Semantics

Margins are expressed in pixels and reduce the sprite's collision rectangle.

For a 16x16 sprite:

```text
        LEFT              RIGHT
          ↓                 ↓
+---------+-----------------+---------+
|         |                 |         |
|         |                 |         |
|         |   collision     |         |
|         |     area        |         |
|         |                 |         |
+---------+-----------------+---------+
          ↑                 ↑
         TOP              BOTTOM
```

The following constraints apply, where `SPRSIZ` is the current effective sprite
size calculated by the runtime:

```text
0 <= LEFT   <= SPRSIZ
0 <= TOP    <= SPRSIZ
0 <= RIGHT  <= SPRSIZ
0 <= BOTTOM <= SPRSIZ

LEFT + RIGHT <= SPRSIZ
TOP  + BOTTOM <= SPRSIZ
```

These constraints prevent the resulting collision rectangle from becoming inverted unintentionally.

A zero-width or zero-height collision area is valid and represents a non-colliding sprite.

---

# 8. Internal Hitbox Representation

The public API is expressed in terms of margins:

```text
LEFT, TOP, RIGHT, BOTTOM
```

However, the runtime should **not store raw margins as its collision representation**.

Instead, the runtime stores pre-normalized **relative** collision bounds:

```text
X0
X1
Y0
Y1
```

For a sprite whose current effective size is `SPRSIZ`:

```text
X0 = LEFT
X1 = SPRSIZ - RIGHT

Y0 = TOP
Y1 = SPRSIZ - BOTTOM
```

This transformation is performed when the hitbox is configured. The table stores
offsets relative to the sprite origin, not absolute screen coordinates.

Consequently, `COLLISION()` does not need to perform margin subtraction such as
`SPRSIZ - RIGHT` or `SPRSIZ - BOTTOM`. It only combines the current sprite
position from `SPRTBL` with the pre-normalized relative bounds.

This representation is preferred over absolute bounds because hitbox
configuration remains stable when a sprite moves. `PUT SPRITE` can continue to
update `SPRTBL` as the current position source of truth without also rewriting
`HITBOX_TABLE` on every movement.

The trade-off is that `COLLISION()` must add the sprite position to the relative
bounds before comparison. This is slightly more work than consuming fully
absolute hitbox bounds, but it avoids movement-time hitbox maintenance and keeps
the `SET SPRITE HITBOX` command persistent until explicitly changed.

The design therefore optimizes for:

```text
stable configuration
+
cheap movement
+
precomputed right/bottom bounds
```

rather than for the absolute minimum number of arithmetic operations in
`COLLISION()`.

---

# 9. Disabled Hitbox Representation

A disabled hitbox is represented by a per-sprite enabled flag stored in the
reserved byte of `HITBOX_TABLE` (see section 10), rather than by a separate
global mode flag.

The originally proposed approach of encoding an empty hitbox as inverted
relative bounds (`X0 = middle`, `X1 = middle - 1`) was found to be
**insufficient**: the AABB test still succeeds when a disabled sprite is
compared against an enabled sprite at the same position. For a disabled sprite
A (relative `X0=8, X1=7`, absolute `ax0=108, ax1=107` at position 100) and a
full 16x16 sprite B at position 100 (`bx0=100, bx1=116`):

```text
BX0 < AX1  ->  100 < 107  (true)
AX0 < BX1  ->  108 < 116  (true)
```

Inverted bounds only suppress collision when *both* sprites are disabled. It is
also impossible to guarantee non-collision using 8-bit relative bounds added to
position, since any sentinel collapses under coordinate wraparound.

Therefore, `HITBOX_TABLE[+4]` is a per-sprite enabled flag: `0xFF` = enabled,
`0x00` = disabled. `COLLISION()` checks this flag once per sprite and skips
disabled sprites. This remains "data over mode flags" — the flag is per-sprite
data in the table, not a global `HITBOX_MODE` switch.

No global:

```text
HITBOX_MODE
```

flag is required by the collision routine.

---

# 10. HITBOX_TABLE

A new runtime table shall be introduced:

```text
HITBOX_TABLE
```

with one entry for each of the 32 sprites.

The recommended physical layout is:

```text
5 bytes per sprite
```

giving:

```text
32 × 5 = 160 bytes
```

per table.

Each entry is:

```text
+0  X0 relative to sprite origin
+1  X1 relative to sprite origin
+2  Y0 relative to sprite origin
+3  Y1 relative to sprite origin
+4  enabled flag (0xFF = enabled, 0x00 = disabled)
```

The fifth byte holds the per-sprite enabled flag (see section 9) and keeps the
same stride as `SPRTBL` so a parallel pointer can traverse both tables in
lockstep.

`SPRTBL` remains unchanged:

```text
SPRTBL
  32 × 5 bytes

HITBOX_TABLE
  32 × 5 bytes
```

This costs 32 additional bytes compared with a compact 4-byte hitbox table, but significantly simplifies and optimizes traversal.

The physical layout should remain an implementation detail of the kernel and must not be exposed to BASIC programs.

`SPRTBL` remains the source of truth for the current sprite position and active
state. `HITBOX_TABLE` remains the source of truth for the configured collision
shape relative to that position.

---

# 11. Parallel Table Traversal

The existing collision algorithm traverses `SPRTBL` using a five-byte stride.

The proposed implementation maintains a corresponding pointer for `HITBOX_TABLE`.

Conceptually:

```text
HL  -> SPRTBL[B]
HL' -> HITBOX_TABLE[B]
```

Both pointers advance by five bytes:

```text
SPRTBL       += 5
HITBOX_TABLE += 5
```

This avoids calculating:

```text
B × 4
```

for every candidate sprite.

It also avoids repeatedly reconstructing a table address from the sprite number.

The hitbox address is therefore determined once and subsequently advanced incrementally.

---

# 12. Collision Hot Path

The most important performance requirement is that hitbox support must not unnecessarily increase the cost of `COLLISION()`.

The existing algorithm loads the bounds of sprite A once and then checks them against each candidate sprite B.

The proposed algorithm follows the same model.

### Sprite A

Its normalized hitbox bounds are loaded once:

```text
E = AX0
D = AX1
C = AY0
B = AY1
```

### Sprite B

For each candidate, the runtime obtains the pre-normalized relative bounds from
`HITBOX_TABLE` and combines them with the current candidate position in `SPRTBL`.

No margin subtraction is performed.

No VRAM access is performed.

No hitbox mode is checked.

No multiplication by the sprite number is performed.

The effective AABB comparison remains conceptually:

```asm
; bx0 < ax1
; ax0 < bx1
; by0 < ay1
; ay0 < by1
```

The objective is to keep this comparison block as close as possible to the
current implementation while accepting the small position-plus-relative-bound
cost introduced by persistent relative hitboxes.

---

# 13. Why the Hitbox Table Stores Relative Bounds

An alternative would be to store:

```text
LEFT
TOP
RIGHT
BOTTOM
```

and calculate the effective rectangle during every collision test.

This is rejected because the collision routine is the hottest part of the feature.

For example, storing margins would require operations equivalent to:

```text
BX0 = sprite_x + LEFT
BX1 = sprite_x + SPRSIZ - RIGHT
BY0 = sprite_y + TOP
BY1 = sprite_y + SPRSIZ - BOTTOM
```

for every candidate.

Instead, configuration-time processing precomputes the relative right/bottom
bounds:

```text
X1 = SPRSIZ - RIGHT
Y1 = SPRSIZ - BOTTOM
```

and stores left/top directly as `X0/Y0`.

The collision routine still adds sprite coordinates from `SPRTBL`, but it does
not need to load raw margins or recalculate `SPRSIZ - margin` for every
candidate.

## 13.1 Why Not Store Absolute Bounds

Fully absolute bounds would be slightly cheaper for `COLLISION()` because the
comparison could consume final screen coordinates directly. However, absolute
bounds become stale whenever `PUT SPRITE` moves a sprite.

That would force movement code to also update `HITBOX_TABLE`, either by
reapplying margins or by computing coordinate deltas. Since sprite movement is a
common operation, this couples hitbox maintenance to every movement and increases
the implementation risk around `PUT SPRITE`.

Relative bounds keep the configured hitbox independent from the current
position:

```text
PUT SPRITE
  -> update SPRTBL only

SET SPRITE HITBOX
  -> update HITBOX_TABLE only

COLLISION()
  -> combine SPRTBL position + HITBOX_TABLE relative bounds
```

This is a better fit for the requirement that hitbox configuration remains
persistent until explicitly changed.

---

# 14. SET SPRITE HITBOX AUTO

The following command is proposed:

```basic
SET SPRITE HITBOX AUTO
```

`AUTO` calculates the hitbox of each of the 32 sprite slots based on the sprite pattern currently associated with that sprite in VRAM.

The operation occurs **only when the command is executed**.

It does not create a dynamic relationship between VRAM and the hitbox table.

Therefore:

```basic
SET SPRITE HITBOX AUTO
```

followed later by changes to sprite patterns does **not** automatically change the hitboxes.

The application must execute:

```basic
SET SPRITE HITBOX AUTO
```

again when a recalculation is required.

---

# 15. Automatic Hitbox Calculation

For each sprite, `AUTO` determines the bounding rectangle of its visible pixels.

Conceptually:

```text
sprite pattern:

................
....██████......
...████████.....
...████████.....
....██████......
................
```

produces:

```text
+----------------+
|                |
|    +-------+   |
|    |███████|   |
|    |███████|   |
|    |███████|   |
|    +-------+   |
|                |
+----------------+
```

The resulting rectangle is converted into the normalized relative:

```text
X0
X1
Y0
Y1
```

representation and stored in `HITBOX_TABLE`.

---

# 16. Fully Transparent Sprites

If `AUTO` determines that a sprite contains no visible pixels, its hitbox is
disabled by clearing its enabled flag (`HITBOX_TABLE[+4] = 0x00`).

Therefore, a fully transparent sprite never participates in a collision.

---

# 17. VRAM Access

VRAM is accessed only by:

```basic
SET SPRITE HITBOX AUTO
```

The collision routines must never read sprite pattern data from VRAM.

This is an important architectural requirement.

The collision path operates exclusively on RAM:

```text
SPRTBL
HITBOX_TABLE
```

This ensures that collision performance remains independent of VRAM access.

---

# 18. Interaction with COLLISION()

No changes are proposed to the BASIC interface of:

```basic
COLLISION()
COLLISION(n)
COLLISION(n1,n2)
```

All existing forms continue to work exactly as before.

The only change is that the collision test uses the effective hitbox represented
by the current `SPRTBL` position plus the relative bounds in `HITBOX_TABLE`.

The existing same-position rule remains unchanged:

> Two sprites at exactly the same X/Y position are treated as the same object and do not collide.

That rule is evaluated independently of hitbox margins.

> **Note on current implementation:** the documented "same X/Y position" rule
> (`openspec/specs/sprites-handling/spec.md`) is **not currently implemented** in
> the collision kernel. The kernel only skips the same sprite *index*
> (`SUB_SPRCOL_ONE`, `cp b; jr z SKIP`); there is no position-equality check.
> Two different sprites at the same position are therefore reported as colliding
> today. Implementing the rule would add a position-equality comparison per
> candidate in the collision hot path. This change **preserves the current
> behavior and does not implement the rule** — it is out of scope here.

---

# 19. Runtime Algorithm

The resulting conceptual algorithm is:

```text
COLLISION(A, B)

1. Check whether A is active.
2. Check whether B is active.
3. Preserve the existing same-position rule.
4. Load A's position from `SPRTBL`.
5. Load A's relative hitbox bounds from `HITBOX_TABLE`.
6. Produce A's effective bounds.
7. Load B's position and relative hitbox bounds.
8. Produce B's effective bounds.
9. Perform the existing AABB test.
10. Return the same collision result format as today.
```

For:

```basic
COLLISION(A)
```

the same operation is repeated for each candidate B.

---

# 20. Compiler Changes

The compiler only needs to recognize the new syntax and emit the corresponding runtime operation.

Examples:

```basic
SET SPRITE HITBOX ON
SET SPRITE HITBOX OFF
SET SPRITE HITBOX AUTO
SET SPRITE HITBOX 3
SET SPRITE HITBOX 3 ON
SET SPRITE HITBOX 3 OFF
SET SPRITE HITBOX 3 AUTO
SET SPRITE HITBOX 3,2,3,2,4
```

No changes are required to the generated code for existing:

```basic
COLLISION()
```

calls.

This is important for compatibility and performance.

Programs that do not use `SET SPRITE HITBOX` continue using the same collision API.

---

# 21. Backward Compatibility

The runtime default is equivalent to:

```basic
SET SPRITE HITBOX ON
```

Therefore:

```text
LEFT   = 0
TOP    = 0
RIGHT  = 0
BOTTOM = 0
```

for all sprites when the runtime starts. Internally this is stored as:

```text
X0 = 0
X1 = SPRSIZ
Y0 = 0
Y1 = SPRSIZ
```

Consequently, the effective collision rectangle is identical to the existing full sprite rectangle.

Existing source programs therefore retain their previous collision semantics when recompiled.

Previously compiled ROMs are unaffected because they contain their existing kernel implementation.

---

# 22. Performance Considerations

Collision detection is a hot path and may execute once for every sprite against up to 31 other sprites.

The implementation should therefore avoid:

* multiplication;
* division;
* VRAM access;
* calls to a hitbox lookup subroutine;
* per-candidate mode checks;
* recalculation of right/bottom margins;
* address calculation from the sprite number;
* changes to the existing `SPRTBL` stride.

The preferred implementation uses:

```text
precomputed relative bounds
+
parallel pointer
+
incremental traversal
```

rather than:

```text
sprite number
→ calculate index
→ calculate address
→ load raw margins
→ calculate SPRSIZ - margins
→ test collision
```

The additional 32 bytes required by the five-byte `HITBOX_TABLE` representation are considered an acceptable trade-off for reducing runtime work.

Relative bounds do not remove all arithmetic from `COLLISION()`: the routine
still needs to combine sprite coordinates with the relative bounds. They do help
by avoiding movement-time updates to the hitbox table and by avoiding repeated
`SPRSIZ - RIGHT/BOTTOM` calculations later.

---

# 23. Alternatives Considered

## 23.1 Four-byte HITBOX_TABLE

```text
32 × 4 = 128 bytes
```

This is memory-efficient but requires a different stride from `SPRTBL`.

It complicates parallel traversal and may require additional address management.

**Rejected in favor of the five-byte representation.**

---

## 23.2 Calculate `4 × sprite` for every candidate

The runtime could calculate:

```asm
sprite * 4
```

and use the result to access `HITBOX_TABLE`.

This adds arithmetic to the collision hot path.

**Rejected.**

---

## 23.3 Use IX/IY indexed addressing

A structure could be accessed through:

```asm
(IY+n)
```

or:

```asm
(IX+n)
```

However, indexed Z80 operations are considerably more expensive than normal register-indirect accesses.

**Rejected for the hot path.**

---

## 23.4 Store margins instead of normalized bounds

This minimizes the conceptual size of the table but requires collision-time calculations.

**Rejected.**

## 23.4.1 Store absolute bounds

Absolute bounds would minimize arithmetic inside `COLLISION()`, but would require
`PUT SPRITE` and any other sprite movement path to update both `SPRTBL` and
`HITBOX_TABLE`.

**Rejected in favor of relative normalized bounds.**

---

## 23.5 Extend SPRTBL to contain hitbox data

Changing:

```text
5 bytes/sprite
```

to:

```text
9 bytes/sprite
```

would eliminate a second table, but would also change the fundamental layout of `SPRTBL`.

That could affect existing kernel routines and compiled code assumptions.

The resulting scope and compatibility risk are not justified.

**Rejected.**

---

## 23.6 Global HITBOX_MODE flag

A global flag could distinguish:

```text
ON
OFF
AUTO
```

during collision detection.

This is unnecessary because the table itself is the source of truth.

**Rejected.**

---

# 24. Source of Truth

`HITBOX_TABLE` is the **strict runtime source of truth** for collision hitboxes.

`COLLISION()` does not need to know whether a hitbox originated from:

```text
SET SPRITE HITBOX ON
SET SPRITE HITBOX OFF
SET SPRITE HITBOX <sprite>
SET SPRITE HITBOX <sprite> ON
SET SPRITE HITBOX <sprite> OFF
SET SPRITE HITBOX <sprite>,...
SET SPRITE HITBOX AUTO
SET SPRITE HITBOX <sprite> AUTO
```

All commands ultimately produce the same internal relative representation:

```text
X0
X1
Y0
Y1
```

This keeps the collision algorithm independent of how the hitbox was configured.

---

# 25. Validation

The compiler/runtime must reject invalid margins:

```text
LEFT   < 0
TOP    < 0
RIGHT  < 0
BOTTOM < 0

LEFT   > SPRSIZ
TOP    > SPRSIZ
RIGHT  > SPRSIZ
BOTTOM > SPRSIZ

LEFT + RIGHT > SPRSIZ
TOP  + BOTTOM > SPRSIZ
```

The valid range is inclusive. For a 16x16 sprite, `SPRSIZ = 16`; for other
sprite modes, the runtime size must be used instead of hardcoding 16.

Therefore:

```basic
SET SPRITE HITBOX 1,8,0,8,0
```

is valid and produces a zero-width collision area.

Likewise:

```basic
SET SPRITE HITBOX 1,0,8,0,8
```

produces a zero-height collision area.

---

# 26. Test Plan

The implementation should include tests for:

### Default behavior

```text
No SET SPRITE HITBOX command
→ full sprite collision rectangle
```

### ON

```text
SET SPRITE HITBOX ON
→ all sprites have zero margins
```

### OFF

```text
SET SPRITE HITBOX OFF
→ no sprite collides
```

### Individual sprite

```text
SET SPRITE HITBOX 3
→ sprite 3 uses full rectangle
```

### Explicit margins

Test all four margins independently and in combination.

### Boundary conditions

```text
LEFT + RIGHT = SPRSIZ
TOP + BOTTOM = SPRSIZ
```

### Invalid values

Negative and out-of-range values must be rejected.

### AUTO

Test:

* completely opaque sprite;
* partially transparent sprite;
* sprite with transparent border;
* sprite with a single visible pixel;
* fully transparent sprite.

### Collision semantics

Verify:

* collision with zero margins;
* collision after shrinking A;
* collision after shrinking B;
* collision behavior remains stable after moving a sprite with `PUT SPRITE`;
* collision disappearing because of hitbox margins;
* collision retained when the effective rectangles overlap;
* same-position sprites remain non-colliding.

### AUTO persistence

Verify:

```text
SET SPRITE HITBOX AUTO
change sprite pattern
COLLISION()
```

does not automatically change the previously calculated hitbox.

A second:

```basic
SET SPRITE HITBOX AUTO
```

must recalculate it.

---

# 27. Example

Consider two 16x16 sprites.

Without hitboxes:

```text
Sprite A: [0,16] × [0,16]
Sprite B: [15,31] × [0,16]
```

They collide because their rectangles overlap by one pixel.

Now:

```basic
SET SPRITE HITBOX 0,2,2,2,2
SET SPRITE HITBOX 1,2,2,2,2
```

produces:

```text
Sprite A: [2,14] × [2,14]
Sprite B: [17,29] × [2,14]
```

The collision disappears.

The application can therefore tune the collision geometry without changing the sprite graphics.

---

# 28. Design Principles

The implementation follows four main principles:

### 1. Configure once, collide cheaply

All expensive hitbox processing occurs when the hitbox is configured.

### 2. RAM over VRAM

Collision detection never depends on VRAM access.

### 3. Data over mode flags

The hitbox table itself determines whether and where a sprite can collide.

### 4. Preserve the existing kernel structure

The existing `SPRTBL` and AABB collision mechanism remain fundamentally unchanged.

---

# 29. Open Implementation Detail

The exact Z80 register allocation used to maintain the parallel `SPRTBL` and `HITBOX_TABLE` pointers is an implementation detail of the kernel.

The implementation should prefer the existing alternate register set where practical and avoid introducing IX/IY into the collision hot path.

The final implementation should be benchmarked against the current collision routine using representative workloads:

```text
1 active sprite
8 active sprites
16 active sprites
32 active sprites
```

with both early collision and full-loop/no-collision cases.

The acceptance criterion is that the new implementation introduces the smallest practical overhead while preserving the current collision algorithm's behavior.

---

# 30. Conclusion

This RFC introduces configurable collision geometry without turning `COLLISION()` into a more complex or stateful operation.

The BASIC programmer works with intuitive margins:

```basic
SET SPRITE HITBOX 3,2,3,2,4
```

while the kernel works with precomputed relative bounds:

```text
X0 X1 Y0 Y1
```

The proposed parallel five-byte `HITBOX_TABLE` deliberately spends a small amount of RAM to avoid additional address arithmetic in the collision hot path.

Most importantly, the feature remains **data-driven**:

```text
SET SPRITE HITBOX
      ↓
precompute relative hitbox
      ↓
HITBOX_TABLE
      ↓
COLLISION()
      ↓
existing AABB test
```

This preserves the simplicity of the current collision model while providing substantially more useful collision behavior for non-rectangular sprites.
