Keyboard Macros Plugin
======================

Record and play keyboard macros (i.e., keyboard action sequences).

## Basic usage

### To start recording a keyboard macro:

Menu: `Tools > Keyboard Macros > Record Macro…`

Default shortcut: `Ctrl+Shift+K`.

The plugin will record every key presses until you end recording.

### To end recording:

Menu: `Tools > Keyboard Macros > End Macro Recording`

Default shortcut: `Ctrl+Shift+K`.

The plugin will stop recording key presses and save the key presses sequences as the current macro.

### To cancel recording:

Menu: `Tools > Keyboard Macros > Cancel Macro Recording`

The plugin will stop recording key presses but the current macro won't change.

Default shortcut: `Alt+Shift+K, D`.

### To play the current macro:

Menu: `Tools > Keyboard Macros > Play Macro`

Default shortcut: `Ctrl+Alt+K`.

The plugin will play the current macro.

## Named macros

It is possible to save keyboard macros by giving them a name.

Named macros are persistent between Kate's sessions, they're saved in the `keyboardmacros.json` file in Kate's user data directory (usually `~/.local/share/kate/`).

### To save the current macro:

Menu: `Tools > Keyboard Macros > Save Current Macro`

Default shortcut: `Alt+Shift+K, S`.

Command: `kmsave <name>` will save the current macro under the name `<name>`.

### To load a saved macro as the current one:

Menu: `Tools > Keyboard Macros > Load Named Macro`

Default shortcut: `Alt+Shift+K, L`.

Command: `kmload <name>` will load the macro saved under the name `<name>` as the current one.

### To play a saved macro without loading it:

Command: `kmplay <name>` will play the macro saved under the name `<name>` without loading it.

### To delete a saved macro:

Default shortcut: `Alt+Shift+K, D`.

Command: `kmdelete <name>` will delete the macro saved under the name `<name>`.

### Tips for commands 

Note that after the `km` prefix all these commands use a different letter so you can efficiently use them using tab-completion :).

## Limitations

As of now, keyboard macros fail to play properly if some types of GUI widgets are used: QMenu, QuickOpenLineEdit, or TabSwitcherTreeView, for example.
I'm not sure why but my first guess would be that it is a combination of these widgets taking time to show up and of the fact that they seem to works differently regarding keyboard events.
