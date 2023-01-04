# COMPARISON BETWEEN THIS FORK AND ITS PARENT PROJECT

For simplicity's sake, I will use **yadL** for the products of this fork, and **yadU** for the parent project's. [More info](README.md)

yadL version number is formed as 0.42.x where integer "x" indicates some released progress worth mentioning.

Numbered notes are grouped together at the end of this long page.

## Licence and copyright

GNU GPL3 is yadL's and yadU's license. Copyright notices in yadL source code are not maintained therefore they aren't up-to-date. However, to comply with GPL3's license I hereby claim the copyright of the source code added to this repository:

		yad maintenance branch, aka yad-maintain-gtk2, yadL, (C) 2019-2023 step

## New features of yadL

yadL of its own adds the following new features not included in yadU, to the best of my knowledge.
Refer to [yadL commit log](https://github.com/step-/yad/commits) for more details.

- option `--multi-progress` is a deprecated alias for `--progress`
- option `--icon-width=SIZE` is a deprecated alias for `--icon-size`
- new option `--center-keep`
- yadL GTK+-3 binary still supports stock items for button icons
- yadL still uses the GtkSpell3 spell engine
- yadL's own about dialog is modified
- save print settings to new file `$XDG_CONFIG_HOME/yad/print.conf` ⁽⁵⁾

## Notable bug fixes

Several bugs that affected the parent project when it was forked have been fixed. The following stand out:

- some print dialog issues⁽¹⁾
- forcefully close a notebook dialog on exit if some tab has hung

## Frozen or untested features

* As Fatdog64 no longer packages the Webkit engine, I did not make an effort to maintain or even test yadL's HTML dialog option.

## Diverged features

See the _Notes_ section.

## Features and bug fixes backported from yadU to yadL

[This list](https://github.com/step-/blob/maintain-gtk2/feature-comparison.md)
of yadU features is extracted from the [NEWS](https://github.com/v1cont/NEWS) file.

The version numbers in the following headings refer to yadU releases.
The version numbers in square brackets refer to yadL releases.

Abbreviations:

* `[+]` the feature is backported therefore available in yadL
* `[>]` the feature will not be backported as it depends on capabilities that are not available in GTK+-2
* `[-]` the feature will not be backported (reason may vary)
* `[!]` the feature will not be backported because it is incompatible with the yadL souce code base (diverged feature)
* `[ ]` the feature is undecided, not yet evaluated for backporting, etc.
* `[?]` the feature is being evaluated for backporting

---

Version 12.3

- `[ ]` fixed setting webkit properties and user defined style sheet in html dialog
- `[ ]` improve stdin handler for html dialog
- `[ ]` update copyright notice

Version 12.2

- `[ ]` add --auto-scroll option as an alias to --tail
- `[ ]` fixed jump to anchor in simple mode of html dialog
- `[?]` fixed parsing data from stdin for tree mode of list dialog
- `[ ]` fixed man page
- `[ ]` code cleanup

Version 12.1

- `[ ]` fixed some typos
- `[ ]` fixed permissions in thumbnails creation
- `[ ]` improve interpreteur string in yad-settings script

Version 12.0

- `[ ]` add ability to load several images in picture dialog
- `[ ]` add color picker to color dialog and yad-tools
- `[ ]` add --line option to text-info dialog
- `[ ]` improve guess of syntax highlighting in text-info dialog
- `[ ]` add --mime option to icon mode in yad-tools
- `[?]` add switch field type in form dialog (thanks to Misko <mpsrbija@gmail.com>)
- `[ ]` use pango markup for multiline text field in form dialog

Version 11.1

- `[ ]` fix input parsing for notification icon
- `[ ]` fix parsing desktop files for icons dialog
- `[ ]` fix link handling for browser mode in html dialog
- `[ ]` improve setting user-defined image in about dialog

Version 11.0

- `[ ]` DROP K HUJAM russian translation
- `[?]` for other changes please discover the git changelog

Version 10.1

- `[ ]` don't make rows homogeneous when form has a text field

Version 10.0

- `[ ]` many improvements in text-info dialog
  - `[?]` uses monospace font by default
  - `[ ]` added search-bar instead of popup search field (can be disabled)
  - `[ ]` added more GtkSourceView capabilities (use --help-source to see full list of those options)
  - `[ ]` added in-place editing and file operations throug popup menu or keybindings
- `[ ]` added search bar to html dialog (can be disabled)
- `[ ]` added Ctrl+O and Ctrl+Q shortcuts to html dialog
- `[ ]` added --f1-action which run command when F1 was pressed
- `[ ]` added --changed-action to form dialog for control states of switcher fields like check or combo boxes
- `[ ]` fixed build with musl
- `[ ]` added yad-settings script. this is yad-based frontend for edit yad settings
- `[ ]` force using small icons in list dialog if icon is not a real filename
- `[ ]` fixed fitting image in picture dialog

Version 9.3

- `[ ]` fix freezing main window on --row-action in list or @cmd in form dialogs

Version 9.2

- `[ ]` some fixes of menu in editable lists
- `[ ]` add two additional item to menu in editable lists - "move up" and "move down"
- `[?]` use double quotes for arguments in default interpreter command

Version 9.1

- `[?]` fix --version option

Version 9.0

- `[ ]` implement user's customizable --about dialog
- `[?]` fix passing focus to children in notebook and paned dialogs
- `[ ]` add --stack mode to notebook dialog
- `[?]` add --focused option to paned dialog for selectin focused pane
- `[?]` don't vertically expand entry in entry dialog
- `[?]` fix setting splitter position in paned dialog
- `[ ]` add --wk-prop to html dialog and made js output enabled by default
- `[-]` add yad-tools utility instead fo pfd. pfd now is a wrapper script (just for backward compatibility)
- `[+]` add --align-buttons option for aligning labels on button fields in form dialog [0.42.50]


- `[?]` add --text-width option for more flexible wrapping long strings in dialog text
- `[ ]` handle URIs in dialog text
- `[>]` add --css option (--gtkrc marked as deprecated)
- `[ ]` lots of bug fixes

Version 7.3

- `[+]` fix parsing separator field for form dialog (thanks to Dmitry Butskoy)
- `[ ]` fix handling user defined size of dialog with --width and --height arguments

Version 7.2

- `[+]` fix parsing field names for form dialog
- `[+]` fix uri-handler for html dialog
- `[ ]` add file: scheme for uri regexp in text-info dialog

Version 7.1

- `[+]` fix parsing field names for form dialog

Version 7.0

- `[ ]` add --enforce-step option to scale dialog
- `[ ]` add tooltips and markup to column headers in list dialog
- `[ ]` add pango-style font definition to text-info dialog
- `[ ]` add --large-preview options for better thumbnails handling
- `[ ]` add --interactive option to icon browser for printing selected icons on stdout
- `[+]` add custom uri handler (--uri-handler option)
- `[+]` improve killing children algo for notebook and paned dialogs
- `[+]` add tooltips for form fields
- `[ ]` fix :tip column in list dialog
- `[ ]` fixes and cleanups in miscellaneous functions
- `[+]` fix parsing numeric ranges in for dialog
- `[ ]` fixes in man page
- `[ ]` build icon browser by default

Version 6.0

- `[+]` add --use-interp option ⁽²⁾
- `[+]` add link field to --form dialog
- `[ ]` returning customizable options for --text-info dialog
- `[ ]` fix yad behavior outside X11 (spetial thanks to Michael Weiser)
- `[ ]` improve dialog window placement
- `[ ]` update translations

Version 5.0

- `[!]` add debug mode. this feature can be turned on through gsettings.
- `[!]` add configure option --enable-standalone for build yad without gsettings support
- `[+]` fix expanding tree nodes for stdin data
- `[+]` some fixes in print dialog
- `[+]` fix --uri-handler option in html dialog
- `[ ]` fix wrong autoclose behavior in progress dialog

Version 4.1

- `[+]` fix handling tree data from stdin in list dialog

Version 4.0

- `[>]` font can be selected with double-click in font dialog
- `[>]` add application chooser dialog and application chooser field in form dialog
- `[+]` add tree mode in list dialog
- `[+]` add --hide-text option to progress dialogs
- `[+]` multi-progress dialog features merged with progress dialog. separate multi-progress dialog no longer exists
- `[>]` enable markup in progress log window
- `[+]` fix output of color values in hex notation
- `[+]` removed \*-selection aliases for --file, --color and --font dialogs ⁽³⁾

Version 3.0

- `[+]` add user defined handlers for all editing actions in list dialog.

- `[?]` fix loading text from file in text-info dialog
- `[!]` fix gsettings key names

Version 2.0

- `[!]` migrate to gsettings from config file
- `[+]` add --keep-icon-size option
- `[>]` icon browser shows only regular icons by default. this behavior can be changed from command line
- `[-]` fix segfault in text-info dialog when empty file is specified ⁽⁴⁾
- `[+]` fix initial selection in selectable labels

Version 1.0

- `[-]` completely removed support of gtk+-2.0
- `[+]` minimum required gtk+ version bump to 3.22.0
- `[>]` add --formatted option to text-info dialog for displaing text with pango markup
- `[+]` add --expand option to tab dialog
- `[ ]` default protocol for html widget is https now
- `[-]` gtk stock items not used anymore. instead there are some yad predefined names. details can be found in man page
- `[+]` add --bool-fmt for different forms of boolean values output. all of this forms recognized on input automatically
- `[!]` gspell library now used for spell checking
- `[?]` tray icon is optional now. this dialog is enabled by default but can be turned off with ./configure --disable-tray
- `[-]` remove --fore, --back and --font options for text-info dialog. those functionality can be done through custom css

Version 0.42.0

- `[+]` add pfd utility for fontnames transformation in scripts
- `[+]` add 'menu' action for notification icon
- `[+]` add --icon-size option to icon dialog
- `[+]` add --simple-tips option to list dialog
- `[+]` fixes in setting window size
- `[+]` improve size scaling in separate output of font dialog

See the NEWS file for older versions.

## Notes

⁽¹⁾ The GTK+-2 package in your distribution may need a patch to fix a long-standing issue whereby the GTK+ "Print to File" printer silently fails to print.  This bug affects other GTK+-2 applications, such as evince, and does not involve CUPS printers.  See this [commit message](https://github.com/step-/yad/commit/c3322e79).

⁽²⁾ Use interpreter option `--use-interp`. yadL default interpreter is `sh -c '%'` whereas it is `bash -c '%'` for yadU.

⁽³⁾ yadU commit removed undocumented aliases for `--color`, `--file` and `--font options`; Fatdog64's scripts did not use the removed options; yadL commit fd5dea9.

⁽⁴⁾ Not applicable.

⁽⁵⁾ Print settings are saved to new file `$XDG_CONFIG_HOME/yad/print.conf`.  Other settings are still saved to `$XDG_CONFIG_HOME/yad.conf`.  Instead, yadU uses `gsettings` to save its configuration; it can be built "standalone" without `gsettings` but then there is no `yad.conf` to save the configuration.

