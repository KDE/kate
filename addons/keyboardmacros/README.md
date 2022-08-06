Keyboard Macros Plugin
======================

Record and play keyboard macros (i.e., keyboard action sequences).

## Basic usage

### To start recording a keyboard macro:

Menu: `Tools > Keyboard Macros > Record Macro...`

The plugin will record every key presses until you end recording.

Default shortcut: `Ctrl+Shift+K`.

### To end recording:

Menu: `Tools > Keyboard Macros > End Macro Recording`

The plugin will stop recording key presses and save the key presses sequences as the current macro.

Default shortcut: `Ctrl+Shift+K`.

### To cancel recording:

Menu: `Tools > Keyboard Macros > Cancel Macro Recording`

The plugin will stop recording key presses but the current macro won't change.

Default shortcut: `Ctrl+Alt+Shift+K`.

### To play the current macro:

Menu: `Tools > Keyboard Macros > Play Macro`

The plugin will play the current macro.

Default shortcut: `Ctrl+Alt+K`.

Command: `kmplay` will play the current macro.

## Named macros

It is possible to save keyboard macros by giving them a name.

Named macros are persistent between Kate's sessions, they're saved in the `keyboardmacros.json` file in Kate's user data directory (usually `~/.local/share/kate/`).

### To save the current macro:

Menu: `Tools > Keyboard Macros > Save Current Macro`

The plugin will prompt you for a name and save the macro under it.

Default shortcut: `Alt+Shift+K`.

Command: `kmsave <name>` will save the current macro under the name `<name>`.

### To load a saved macro as the current one:

Menu: `Tools > Keyboard Macros > Load Named Macro... >`

The plugin will list saved macros as items in this submenu, activating an item will load the corresponding macro as the current one.

Command: `kmload <name>` will load the macro saved under the name `<name>` as the current one.

### To play a saved macro without loading it:

Menu: `Tools > Keyboard Macros > Play Named Macro... >`

The plugin will list saved macros as items in this submenu, activating an item will play the corresponding macro without loading it.

Command: `kmplay <name>` will play the macro saved under the name `<name>` without loading it.

### To wipe (e.g., delete) a saved macro:

Menu: `Tools > Keyboard Macros > Wipe Named Macro... >`

The plugin will list saved macros as items in this submenu, activating an item will wipe the corresponding macro.

Command: `kmwipe <name>` will delete the macro saved under the name `<name>`.

### Tips for commands 

Note that after the `km` prefix all these commands use a different letter so you can efficiently call them using tab-completion :).

## Limitations

As of now, keyboard macros fail to play properly if some types of GUI widgets are used: QMenu, QuickOpenLineEdit, or TabSwitcherTreeView, for example.
I'm not sure why but my first guess would be that these widgets work in a non-standard way regarding keyboard events.
