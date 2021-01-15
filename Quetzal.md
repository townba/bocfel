# Quetzal Extensions

Bocfel extends Quetzal for two purposes.

The first is to add history to a save file to provide context on
restore: the state of the screen (lower window only) at the time of save
is shown on restore. This uses a new IFF chunk type (called `Bfhs`) and
is completely compatible with other interpreters as they will ignore
unknown chunks.

The second is to allow meta saves, or interpreter-provided saves. Bocfel
provides ephemeral saves (using `/ps`) as well as disk-backed saves
(`/save`) for games which do not support saving. However, these save
files are no longer Quetzal files, because they operate slightly
differently due to the game not handling the save code. Instead of the
IFF type `IFZS` as in Quetzal, these files are `BFZS`.

# History

A `Bfhs` chunk represents part of the state of the screen at the time
the story was saved. Up to 2000 history events are stored for playback
after a restore, with a history event being, roughly, a style or a
character. The general format of the chunk is:

* Version (32-bit) - This section describes version 0
* History entry type (8-bit)
* History entry data (size depends on type)

## History entry types

### Style

This is type 0. A style entry switches the text style to the specified
value. This 8-bit value is exactly equivalent to the Z-machine's styles,
which have the following values:

* 0: No style
* 1: Reverse video
* 2: Bold
* 4: Italic
* 8: Fixed-width

These can be combined by adding/bitwise ORing them together.

### Colors

Foreground color is type 1, background is type 2. Colors are specified
by two following values. The first is an 8-bit mode and the second a
16-bit value. The mode takes one of two values:

* 0: ANSI color
* 1: True color

For ANSI color, the 16-bit value is as described in §8.3.1 of the
Z-machine Standards Document 1.1, with the restriction that it be in the
range 1-12.

For true color, the value is a 15-bit true color value as described in
§8.3.7 of the Z-machine Standards Document 1.1.

### Start of Input

This is type 3, and marks the start of user input. This allows the
interpreter to style the input appropriately (such as with `style_Input`
in Glk). There is no associated data, as this is only a marker.

### End of Input

This is type 4 and is a complement to the *Start of Input* marker.
There is no associated data.

### Character

This is type 5 and is a character from the history, either printed by
the game or entered by the user, but only for the lower window.
Characters are stored in UTF-8, meaning the size is variable.

# Meta Save Format

When a game wants to save, it calls the `@save` opcode, which it knows
will be restored from using `@restore`. The program counter is stored by
Quetzal such that it is inside the `@save`, to be continued on
`@restore`. For meta saves, there is no `@save` call, so the program
counter cannot be there. Instead, the user can request a meta save
during a `@read` call, and as such, the only place the program counter
can be stored is at or around that `@read` call. Because the read was
interrupted to perform the save, the interpreter needs to restart the
read call on restore. In short, saving and restoring happens entirely
inside of `@read` which is not something Quetzal can do. Creating the
new `BFZS` type ensures that other interpreters do not try to load these
files.

In addition, two new chunks are defined to handle extra information
needed during a meta save.

## The `Args` Chunk

When `@read` is restarted after a restore, the arguments that were
originally provided to it (at the time of save) must be restored as
well: there is no guarantee that the arguments at the current `@read`
are the same as they were during the save. This chunk stores the
arguments.

The size of this chunk is the number of arguments multiplied by two,
followed directly by the arguments, each a 16-bit value. The number of
arguments can be inferred from the size of the chunk.

## The `Scrn` Chunk

The state of the screen at the time of the save needs to be restored as
well. Note that this is orthogonal to the history chunk. The state of
the screen consists of items such as the size of the upper window, the
current font, and so on. At the point of restore there is no way to be
certain the game's screen is in the same state as it was before, and
since the game is not performing a restore, it has no way of knowing
that it needs to perform actions such as opening the upper window, and
so on.

The chunk size is variable, based on the Z-machine version, and consists
of:

* Version (32-bit) - This section describes version 0
* Currently-selected window (8-bit) - Ranges from 0-7
* Height of the upper window (16-bit) - 0 if it is closed
* The X coordinate of the upper window cursor (16-bit)
* The Y coordinate of the upper window cursor (16-bit)
* Window data (described below)

Coordinates are stored as Z-machine coordinates, meaning (1, 1) is the
origin. If no coordinates are available (e.g. if there is no upper
window), they will be (0, 0).

This chunk is optional. If it is not present, the current screen state
will be maintained. More importantly, unsupported `Scrn` versions are
not fatal. This allows older versions of Bocfel to load newer meta saves
that include updated versions of the `Scrn` chunk.

### Window Data

A window is represented as follows:

* Style (8-bit) - As in `@set_text_style`
* Font (8-bit) - As in §8.1.2 and `@set_font`
* Foreground color (24-bit) - See *Colors* above
* Background color (24-bit) - See *Colors* above

For V6 story files, the window data consists of 8 windows. For all
others, it consists of 2. The first entry is window 0, the second is
window 1, and so on.

This window data is obviously insufficient for V6, as it doesn't include
most of the window properties in §8.8.3.2. This is chiefly because
Bocfel doesn't really support V6. If V6 support is ever added, the
`Scrn` chunk will be updated to a new version with better V6 window
support.
