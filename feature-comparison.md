# COMPARISON BETWEEN THIS FORK AND ITS PARENT PROJECT

For short, I will use **yadL** for the products of this fork, and **yadU** for the parent project's. [More info](README.md)

yadL version number is formed as 0.42.x where integer "x" indicates some released progress worth mentioning.

Numbered notes are grouped together at the end of this long page.

## Licence and copyright

GNU GPL3 is license to both yadL and yadU. Copyright notices in yadL source code are not always up-to-date. To comply with GPL3's license I hereby claim copyright on the source code added to this repository:

		yad maintenance branch, aka yad-maintain-gtk2, yadL, (C) 2019-2023 step

## New features of yadL

yadL of its own adds the following new features not included in yadU, to the best of my knowledge.
Refer to [yadL commit log](https://github.com/step-/yad/commits) for more details.

- option `--icon-width=SIZE` is a deprecated alias for `--icon-size`
- new option `--center-keep`
- yadL GTK+-3 binary still supports stock items for button icons
- yadL still uses the GtkSpell3 spell engine
- yadL's own about dialog is modified
- save print settings to new file `$XDG_CONFIG_HOME/yad/print.conf` ⁽⁵⁾

(§)N Refers to our pull request (PR) number N [https://github.com/v1cont/yad/pulls?q=author%3Astep-](https://github.com/v1cont/yad/pulls?q=author%3Astep-).

- symbolic ID field references in form dialog actions [0.42.71]  (§)213
- symbolic ID names in form dialog output [0.42.71]  (§)213
- auto-disable form buttons and list items while synchronous action runs [0.42.73]  (§)221

## Notable bug fixes

Several bugs that affected the parent project when it was forked have been fixed. The following stand out:

* some print dialog issues⁽¹⁾ [0.42.38]
* forcefully close a notebook dialog on exit if some tab has hung
* unpredictably, notebook tabs are too small [0.42.43]

## Frozen or untested features

* HTML dialog widget - As Fatdog64 Linux no longer packages the Webkit engine, I do not make an effort to maintain or even compile yadL's HTML dialog widget.

## Diverged features

See the _New features of yadL_ and _Notes_ sections.

## Features and bug fixes backported from yadU to yadL

[This list](https://github.com/step-/blob/maintain-gtk2/feature-comparison.md)
of yadU features is extracted from the [NEWS](https://github.com/v1cont/NEWS) file.

The version numbers in the following headings refer to yadU releases.
My annotations are wrapped in square brackets. The version numbers refer to yadL releases.
(§)N Refers to our pull request (PR) number N [https://github.com/v1cont/yad/pulls?q=author%3Astep-](https://github.com/v1cont/yad/pulls?q=author%3Astep-).

Abbreviations:

* `[+]` the feature is backported therefore available in yadL
* `[>]` the feature will not be backported as it depends on capabilities that are not available in GTK+-2
* `[-]` the feature will not be backported (reason may vary)
* `[!]` the feature will not be backported because it is incompatible with the yadL souce code base (diverged feature)
* `[ ]` the feature is undecided, not yet evaluated for backporting, etc.
* `[?]` the feature is being evaluated for backporting

---

Version 12.4?  (yadU WIP)

- `[+]` d7ded23 improve run\_command\_sync for avoiding possible race conditions [0.42.67]
- `[+]` 1d402f7 add some null-pointer checks in form dialog code [0.42.67]
- `[+]` ff81778 remove some warnings [0.42.66]
- `[+]` c8e9ef2 fix UI not updated when sync command runs [0.42.66]  (§)210

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
- `[+]` add color picker to color dialog and yad-tools [0.42.68],[0.42.72]
- `[ ]` add --line option to text-info dialog
- `[ ]` improve guess of syntax highlighting in text-info dialog
- `[+]` add --mime option to icon mode in yad-tools ⁽¹¹⁾ [0.42.68]
- `[+]` add switch field type in form dialog (thanks to Misko <mpsrbija@gmail.com>) [0.42.54]
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

- `[+]` don't make rows homogeneous when form has a text field [0.52.53] +form option --homogeneous

Version 10.0

- `[ ]` many improvements in text-info dialog
  - `[?]` uses monospace font by default
  - `[ ]` added search-bar instead of popup search field (can be disabled)
  - `[ ]` added more GtkSourceView capabilities (use --help-source to see full list of those options)
  - `[ ]` added in-place editing and file operations throug popup menu or keybindings
- `[ ]` added search bar to html dialog (can be disabled)
- `[ ]` added Ctrl+O and Ctrl+Q shortcuts to html dialog
- `[ ]` added --f1-action which run command when F1 was pressed
- `[+]` added --changed-action to form dialog for control states of switcher fields like check or combo boxes [0.42.52]
- `[ ]` fixed build with musl
- `[ ]` added yad-settings script. this is yad-based frontend for edit yad settings
- `[ ]` force using small icons in list dialog if icon is not a real filename
- `[ ]` fixed fitting image in picture dialog

Version 9.3

- `[+]` fix freezing main window on --row-action in list or @cmd in form dialogs [0.42.56]

Version 9.2

- `[ ]` some fixes of menu in editable lists
- `[ ]` add two additional item to menu in editable lists - "move up" and "move down"
- `[+]` use double quotes for arguments in default interpreter command [0.42.61]

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
- `[+]` replace yad-tools for pfd utility. pfd remains as a compatibility wrapper script [0.42.68]
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
- `[+]` fix uri-handler for html dialog [0.42.46]
- `[ ]` add file: scheme for uri regexp in text-info dialog

Version 7.1

- `[+]` fix parsing field names for form dialog

Version 7.0

- `[+]` add --enforce-step option to scale dialog [0.42.62]
- `[+]` add tooltips and markup to column headers in list dialog [0.42.63]
- `[-]` add pango-style font definition to text-info dialog ⁽⁷⁾
- `[+]` add --large-preview options for better thumbnails handling ⁽⁵⁾⁽⁹⁾ [0.42.64]
- `[+]` add --interactive option to icon browser to print selected icon to stdout [0.42.66]
- `[+]` add custom uri handler (--uri-handler option) [0.42.46]
- `[+]` improve killing children algo for notebook and paned dialogs
- `[+]` add tooltips for form fields [0.42.48]
- `[+]` fix :tip column in list dialog [0.42.66]
- `[+]` fixes and cleanups in miscellaneous functions [0.42.46]
- `[+]` fix parsing numeric ranges in form dialog
- `[+]` fixes in man page
- `[+]` build icon browser by default [0.42.60]

Version 6.0

- `[+]` add --use-interp option ⁽²⁾ [0.42.45]
- `[+]` add link field to --form dialog [0.42.47]
- `[-]` returning customizable options for --text-info dialog ⁽⁷⁾ [0.42.44]
- `[+]` fix yad behavior outside X11 (special thanks to Michael Weiser) ⁽⁸⁾ [0.42.59]
- `[?]` improve dialog window placement [8b0e5af]
- `[+]` update translations

Version 5.0

- `[+]` add debug mode. this feature can be turned on through gsettings. ⁽⁵⁾ [0.42.55]
- `[!]` add configure option --enable-standalone for build yad without gsettings support ⁽⁵⁾
- `[+]` fix expanding tree nodes for stdin data
- `[+]` some fixes in print dialog [0.42.38]
- `[+]` fix --uri-handler option in html dialog [0.42.46]
- `[+]` fix wrong autoclose behavior in progress dialog [0.42.57]

Version 4.1

- `[+]` fix handling tree data from stdin in list dialog [0.42.35]

Version 4.0

- `[>]` font can be selected with double-click in font dialog
- `[>]` add application chooser dialog and application chooser field in form dialog
- `[+]` add tree mode in list dialog [0.42.33] -> [0.42.35]
- `[+]` add --hide-text option to progress dialogs [0.42.31]
- `[+]` multi-progress dialog features merged with progress dialog. Separate multi-progress dialog no longer exists ⁽⁶⁾ [0.42.42]
- `[>]` enable markup in progress log window
- `[+]` fix output of color values in hex notation [2e75f42]
- `[+]` removed \*-selection aliases for --file, --color and --font dialogs ⁽³⁾

Version 3.0

- `[+]` add user defined handlers for all editing actions in list dialog.

- `[!]` fix loading text from file in text-info dialog
- `[!]` fix gsettings key names

Version 2.0

- `[!]` migrate to gsettings from config file ⁽⁵⁾
- `[+]` add --keep-icon-size option [0.42.26]
- `[+]` icon browser shows only non-symbolic icons by default [0.42.66] ⁽¹⁰⁾
- `[>]` fix segfault in text-info dialog when empty file is specified ⁽⁴⁾
- `[+]` fix initial selection in selectable labels

Version 1.0

- `[-]` completely removed support of gtk+-2.0
- `[+]` minimum required gtk+ version bump to 3.22.0
- `[>]` add --formatted option to text-info dialog for displaing text with pango markup
- `[+]` add --expand option to tab dialog
- `[+]` default protocol for html widget is https now
- `[-]` gtk stock items not used anymore. instead there are some yad predefined names. details can be found in man page
- `[+]` add --bool-fmt for different forms of boolean values output. all of this forms recognized on input automatically
- `[!]` gspell library now used for spell checking
- `[+]` tray icon is optional now. this dialog is enabled by default but can be turned off with ./configure --disable-tray [0.42.58]
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

(§)N Refer our pull request (PR) number N [https://github.com/v1cont/yad/pulls?q=author%3Astep-](https://github.com/v1cont/yad/pulls?q=author%3Astep-)

⁽¹⁾ The GTK+-2 package in your distribution may need a patch to fix a long-standing issue whereby the GTK+ "Print to File" printer silently fails to print.  This bug affects other GTK+-2 applications, such as evince, and does not involve CUPS printers.  See this [commit message](https://github.com/step-/yad/commit/c3322e79).

⁽²⁾ Use interpreter option `--use-interp`. yadL default interpreter is `sh -c '%'` whereas it is `bash -c '%'` for yadU.

⁽³⁾ yadU commit removed undocumented aliases for `--color`, `--file` and `--font options`; Fatdog64's scripts did not use the removed options; yadL commit fd5dea9.

⁽⁴⁾ Not applicable.

⁽⁵⁾ yadL saves print settings to new file `$XDG_CONFIG_HOME/yad/print.conf`, and other settings to `$XDG_CONFIG_HOME/yad.conf`.  Instead, yadU uses `gsettings` to save its configuration; it can be built "standalone" without `gsettings` but then there is no `yad.conf` to save the configuration.

⁽⁶⁾ yadL still provides option `--multi-progress` as a deprecated alias for `--progress`.

⁽⁷⁾ yadU changed the `--fontname` format of text dialog from Pango style to CSS style [fe11080]+[4b54d14] then back to Pango style [244919d]. yadL has always used Pango style [3858704].

⁽⁸⁾ GTK+-2 does not provide a GDK Wayland backend.  yadU and yadL equally support Wayland for GTK+-3 builds but some dialog features may not be available. If configuration option `debug` is enabled, yadL and yadU will print a warning when an unavailable feature is requested.  To run yadL/yadU in this way, select the Wayland backend by setting `GDK_BACKEND=wayland,x11` in the application's environment. Special value `GDK_BACKEND=help` displays the available backends.

⁽⁹⁾ GTK+-2 generates the preview image when a file item is selected interactively.  Therefore `yad --file --filename=/my/pic.jpg --file --preview [--large-preview]` will not start with the preview of pic.jpg loaded.

⁽¹⁰⁾ GTK+-2 cannot discriminate symbolic icons.  Therefore yad icon browser options `--all` and `--symbolic` are ignored.

⁽¹¹⁾ GTK+-2 `yad-tools --icon --size` always prints the scalable icon name, if available, regardless of the requested size; a similar remark applies to options `--icon --type`.
