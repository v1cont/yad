# Comparing upstream master vs maintain-gtk2

## New features in maintain-gtk2

Of its own this branch adds the following new features vs the upstream project.
Refer to the [commit log](https://github.com/step-/yad/commits) for more details.

- forcefully close hung notebook tabs
- option --multi-progress - deprecated alias for --progress
- option --icon-width=SIZE - deprecated alias for --icon-size
- option --center-keep
- modified about dialog
- fix some print dialog issues
- other bug fixes

**Notes**

**1** Print settings are saved to the new file `$XDG_CONFIG_HOME/yad/print.conf`.
Other settings are still saved to `$XDG_CONFIG_HOME/yad.conf` while the
upstream yad has switched to `gsettings` (and can be built without `gsettings`
but no `yad.conf` to replace `gsettings`).

**2** The GTK+ 2 package in your distro may need a patch to fix a long-standing
issue whereby the GTK "Print to File" printer silently fails printing.  This
bug affects other GTK 2 applications, such as evince, and does not involve CUPS
printers.  See this [commit message](https://github.com/step-/yad/commit/c3322e79).

## Backported features and bug fixes

[This list](https://github.com/step-/blob/maintain-gtk2/feature-comparison.md)
is extracted from the upstream [NEWS](https://github.com/v1cont/NEWS) file.

Note about version numbers.  This fork is versioned with the major.minor number
frozen at the split from upstream, `0.42`, followed by a counter of the
"significant" commits in this fork. This fork makes no formal releases but a
counter bump can be considered as such. The version numbers below refer to
upstream releases.

---

Version 8.0

- [ ] add --text-width option for more flexible wrapping long strings in dialog text
- [ ] handle URIs in dialog text
- [ ] add --css option (--gtkrc marked as deprecated) - **needs gtk3**
- [ ] lots of bug fixes

Version 7.3

- [ ] fix parsing separator field for form dialog (thanks to Dmitry Butskoy)
- [ ] fix handling user defined size of dialog with --width and --height arguments

Version 7.2

- [ ] fix parsing field names for form dialog
- [ ] fix uri-handler for html dialog
- [ ] add file: scheme for uri regexp in text-info dialog

Version 7.1

- [ ] fix parsing field names for form dialog

Version 7.0

- [ ] add --enforce-step option to scale dialog
- [ ] add tooltips and markup to column headers in list dialog
- [ ] add pango-style font definition to text-info dialog
- [ ] add --large-preview options for better thumbnails handling
- [ ] add --interactive option to icon browser for printing selected icons on stdout
- [ ] add custom uri handler (--uri-handler option)
- [x] improve killing children algo for notebook and paned dialogs
- [ ] add tooltips for form fields
- [ ] fix :tip column in list dialog
- [ ] fixes and cleanups in miscellaneous functions
- [ ] fix parsing numeric ranges in for dialog
- [ ] fixes in man page
- [ ] build icon browser by default

Version 6.0

- [ ] add --use-interp option. this feature can reduce quoting in command arguments
- [?] add link field to --form dialog
- [?] returning customizable options for --text-info dialog
- [ ] fix yad behavior outside X11 (spetial thanks to Michael Weiser)
- [ ] improve dialog window placement
- [ ] update translations

Version 5.0

- [ ] add debug mode. this feature can be turned on through gsettings. - **N/A**
- [ ] add configure option --enable-standalone for build yad without gsettings support - **N/A**
- [x] fix expanding tree nodes for stdin data
- [x] some fixes in print dialog
- [ ] fix --uri-handler option in html dialog
- [ ] fix wrong autoclose behavior in progress dialog

Version 4.1

- [x] fix handling tree data from stdin in list dialog

Version 4.0

- [ ] font can be selected with double-click in font dialog - **needs gtk3**
- [ ] add application chooser dialog and application chooser field in form dialog - **needs gtk3**
- [x] add tree mode in list dialog
- [x] add --hide-text option to progress dialogs
- [x] multi-progress dialog features merged with progress dialog. separate multi-progress dialog no longer exists
- [ ] enable markup in progress log window - **N/A**
- [x] fix output of color values in hex notation
- [x] removed \*-selection aliases for --file, --color and --font dialogs - **breaking change fd5dea9dd3**

Version 3.0

- [x] add user defined handlers for all editing actions in list dialog. --add-action parameter renamed to --row-action - **breaking change 8648e3e4a8**

- [ ] fix loading text from file in text-info dialog - **N/A**
- [ ] fix gsettings key names - **N/A**

Version 2.0

- [ ] migrate to gsettings from config file
- [x] add --keep-icon-size option
- [ ] icon browser shows only regular icons by default. this behavior can be changed from command line - **needs gtk3**
- [ ] fix segfault in text-info dialog when empty file is specified - **N/A**
- [x] fix initial selection in selectable labels

Version 1.0

- [ ] completely removed support of gtk+-2.0 - **needs gtk3**
- [ ] minimum required gtk+ version bump to 3.22.0 - **needs gtk3**
- [ ] add --formatted option to text-info dialog for displaing text with pango markup - **needs gtk3**
- [x] add --expand option to tab dialog
- [?] default protocol for html widget is https now
- [ ] gtk stock items not used anymore. instead there are some yad predefined names. details can be found in man page - **needs gtk3**
- [x] add --bool-fmt for different forms of boolean values output. all of this forms recognized on input automatically
- [ ] gspell library now used for spell checking
- [ ] tray icon is optional now. this dialog is enabled by default but can be turned off with ./configure --disable-tray
- [ ] remove --fore, --back and --font options for text-info dialog. those functionality can be done through custom css - **needs gtk3**

Version 0.42.0

- [x] add pfd utility for fontnames transformation in scripts
- [x] add 'menu' action for notification icon
- [x] add --icon-size option to icon dialog
- [x] add --simple-tips option to list dialog
- [x] fixes in setting window size
- [x] improve size scaling in separate output of font dialog

See the NEWS file for older versions.
