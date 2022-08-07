Keyboard Macros Plugin
======================

Introduction
------------

Record and play keyboard macros (i.e., keyboard action sequences).

Basic usage
-----------

### To start recording a keyboard macro:

[Tools \> Keyboard Macros \> Record Macro\...]
[`Ctrl+Shift+K`].

The plugin will record every key presses until you end recording.

### To end recording:

[Tools \> Keyboard Macros \> End Macro Recording]
[`Ctrl+Shift+K`].

The plugin will stop recording key presses and save the sequence as the
current macro.

### To cancel recording:

[Tools \> Keyboard Macros \> Cancel Macro Recording]
[`Ctrl+Alt+Shift+K`].

The plugin will stop recording key presses but the current macro won\'t
change.

### To play the current macro:

[Tools \> Keyboard Macros \> Play Macro]
[`Ctrl+Alt+K`].

The plugin will play the current macro.

The `kmplay` command without any arguments will also play the current
macro.

Named macros
------------

It is possible to save keyboard macros by giving them a name.

Named macros are persistent between Kate\'s sessions, they\'re saved in
the `keyboardmacros.json` file in Kate\'s user data directory (usually
`~/.local/share/kate/`).

### To save the current macro:

[Tools \> Keyboard Macros \> Save Current Macro]
[`Alt+Shift+K`].

The plugin will prompt you for a name and save the macro under it.

The `kmsave name` command will save the current macro under the name
`name`.

### To load a saved macro as the current one:

[Tools \> Keyboard Macros \> Load Named Macro\...].

The plugin lists saved macros as items in this submenu, activating an
item will load the corresponding macro as the current one.

The `kmload name` command will load the macro saved under the name
`name` as the current one.

### To play a saved macro without loading it:

[Tools \> Keyboard Macros \> Play Named Macro\...].

The plugin lists saved macros as items in this submenu, activating an
item will play the corresponding macro without loading it.

Note that each saved macros is an action that is part of the current
action collection so that a custom shortcut can be assigned to it
through the [Settings \> Configure Keyboard Shortcuts\...]
interface.

The `kmplay name` command will play the macro saved under the name
`name` without loading it.

### To wipe (i.e., delete) a saved macro:

[Tools \> Keyboard Macros \> Wipe Named Macro\...].

The plugin lists saved macros as items in this submenu, activating an
item will wipe (i.e., delete) the corresponding macro.

The `kmwipe name` command will wipe the macro saved under the name
`name`.

### Tips for commands:

Note that after the `km` prefix, all these commands use a different
letter so you can efficiently call them using tab-completion!

Limitations
-----------

As of now, keyboard macros fail to play properly if some types of GUI
widgets are used: QMenu, QuickOpenLineEdit, or TabSwitcherTreeView, for
example. I\'m not sure why but my first guess would be that these
widgets work in a non-standard way regarding keyboard events.
