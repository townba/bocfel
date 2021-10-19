# Quetzal Extensions

Bocfel extends Quetzal for several purposes, explained below.

# History

Bocfel adds history to a save file to provide context on restore: the
state of the screen (lower window only) at the time of save is shown on
restore. This uses a new IFF chunk type (called `Bfhs`) and is
completely compatible with other interpreters as they will ignore
unknown chunks.

A `Bfhs` chunk represents part of the state of the screen at the time
the story was saved. Up to 2000 history entries are stored for playback
after a restore, with a history entry being, roughly, a style or a
character. The general format of the chunk is:

* Version (32-bit) - This section describes version 0
* Number of history entries (32-bit)
* History entries (described below)

## History entry

A history entry consists of two values:

* Type (8-bit)
* Data (size depends on type)

### History entry types

#### Style

This is type 0. A style entry switches the text style to the specified
value. This 8-bit value is exactly equivalent to the Z-machine's styles,
which have the following values:

* 0: No style
* 1: Reverse video
* 2: Bold
* 4: Italic
* 8: Fixed-width

These can be combined by adding/bitwise ORing them together.

#### Colors

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

#### Start of Input

This is type 3, and marks the start of user input. This allows the
interpreter to style the input appropriately (such as with `style_Input`
in Glk). There is no associated data, as this is only a marker.

#### End of Input

This is type 4 and is a complement to the *Start of Input* marker.
There is no associated data.

#### Character

This is type 5 and is a character from the history, either printed by
the game or entered by the user, but only for the lower window.
Characters are stored in UTF-8, meaning the size is variable.

# Persistent transcripting

Bocfel provides a persistent transcript, which, if enabled, stores a
game transcript in memory. This is identical to the transcripts produced
when the Z-machine's transcripting is turned on. The persistent
transcript will be written to save files so that transcripting can
persist across sessions without any user intervention. To do this,
persistent transcripts are stored in save files in a `Bfts` chunk.

The chunk size is variable, and consists of:

* Version (32-bit) - This section describes version 0
* The transcript itself (variable)

The transcript is just text encoded in UTF-8. Its size can be inferred
from the chunk size.

# Note taking

Bocfel allows the user to take notes during a game session. These notes
are stored in save files using the `Bfnt` chunk. Notes have no imposed
structure, but are instead simply a collection of arbitrary bytes that
the user can edit in whatever editor is configured. As such, they are
stored directly in the save file with no translation of any kind. A user
could, in theory, embed any file at all as a note, limited only by the
editor (and available memory).

A note chunk consists of:

* Version (32-bit) - This section describes version 0
* The note data (variable)

Notes are versioned in case it turns out to be useful to impose some
sort of structure on the notes. The size of the note data is inferred
from the size of the chunk (i.e. the note data is the size of the chunk
minus 4).

# Meta Save Format

Bocfel allows for interpreter-provided saves, independent from the
Z-machine's `@save` opcode. There are ephemeral saves (using `/ps`) as
well as disk-backed saves (`/save`) for games which do not support
saving. However, these save files are no longer Quetzal files, because
they operate slightly differently due to the game not handling the save
code. Instead of the IFF type `IFZS` as in Quetzal, these files are
`BFZS`.

When a game wants to save, it calls the `@save` opcode, which it knows
will be restored from using `@restore`. The program counter is stored by
Quetzal such that it is inside the `@save`, to be continued on
`@restore`. For meta saves, there is no `@save` call, so the program
counter cannot be there. Instead, the user can request a meta save
during a `@read` call, and as such, the only place the program counter
can be stored is at or around that `@read` call. For `@aread`, this
means the program counter is at the store variable, and for `@sread` it
is actually at the following instruction.

Because the read was interrupted to perform the save, the interpreter
needs to restart the read call on restore. In short, saving and
restoring happens entirely inside of `@read` which is not something
Quetzal can do. Creating the new `BFZS` type ensures that other
interpreters do not try to load these files.

Autosaves will also save at `@read_char`, which operates the same as
saves at `@read`.

In addition, two new chunks are defined to handle extra information
needed during a meta save.

## The `Args` Chunk

When `@read` or `@read_char` is restarted after a restore, the arguments
that were originally provided to it (at the time of save) must be
restored as well: there is no guarantee that the arguments to the
current opcode are the same as they were during the save. This chunk
stores the arguments.

The first part of the chunk is an 8-bit value which specifies the opcode
type: 0 is `@read`, 1 is `@read_char`.

The second part consists of the arguments to the opcode. Each argument
is a 16-bit value. The number of arguments can be inferred from the
chunk size.

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

# Autosaves

Bocfel provides an autosave feature, which saves the game if the
interpreter is shut down (without `@quit` having been called), to be
restored upon the next start. Saves created through the autosave
mechanism are identical to meta saves, but with added chunks.

In order to continue the session in as similar a way as it was when it
left off, three new chunks are defined to handle state which must
persist for autosaves, but which must not persist for other saves.

## The `Rand` Chunk

If the random number generator is in predicable mode (i.e. `@random` has
been called with a negative value), the current state of the PRNG will
be stored in the `Rand` chunk for autosaves. If the game is in random
mode, then this chunk is omitted. Random state is stored as follows:

* PRNG type (16-bit) - The only supported PRNG type is Xorshift32,
                       which has a type of 0.
* PRNG state (described below)

For Xorshift32, the state is a single 32-bit value which represents the
current Xorshift32 value.

## The `Undo` Chunk

For autosaves, the current state of Undo is stored so that after an
autorestore, the UNDO command will work. For games which do not support
undo, the meta command /UNDO will still work.

The chunk size is variable, and consists of:

* Version (32-bit) - This section describes version 0
* Number of undo states (32-bit)
* Undo data (described below)

### Undo data

Undo states are stored sequentially, with the oldest first. Save states
are BFZS data as described by this document. Each save state consists
of:

* Save type (8-bit) - 0 for normal undo, 1 for meta
* Size of the BFZS data (32-bit)
* The BFZS data itself (variable)

## The `MSav` Chunk

In-memory saves (those created by `/ps`) are also included when
autosaving. These are similar to `Undo` chunks.

The chunk size is variable, and consists of:

* Version (32-bit) - This section describes version 0
* Number of in-memory saves (32-bit)
* In-memory save data (described below)

### In-memory save data

In-memory saves are stored sequentially, with the oldest first.
In-memory saves are BFZS data as described by this document. Each
in-memory save consists of:

* Size of the description (32-bit)
* The description (variable)
* Size of the BFZS data (32-bit)
* The BFZS data itself (variable)

In-memory saves are accompanied by a description given by the user. This
description is saved as a UTF-8 string, the size of which (in bytes, not
characters) is provided, followed by the string itself, without a null
terminator.
