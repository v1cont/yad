Yet Another Dialog
====================

Program allows you to display GTK+ dialog boxes from command line or 
shell scripts. YAD depends on GTK+ only. Minimal GTK+ version is 3.22.0

This software is licensed under the GPL v.3

Project homepage: https://github.com/v1cont/yad  
Complex examples: https://sanana.kiev.ua/index.php/yad  
Mailing list: http://groups.google.com/group/yad-common  

Some miscellaneous stuff can be found in data/misc directory including notify-send script
and simple zenity-compatible wrapper 

A fresh gtk2 branch of YAD can be obtained from this repository - https://github.com/step-/yad

Building git version
----------------------

Get git version with command

git clone https://github.com/v1cont/yad.git yad-dialog-code

Before run the standard ./configure && make && make install procedure
you need to generate build scripts. This can be done by running command

autoreconf -ivf && intltoolize

You must manually run gtk-update-icon-cache after installation.

For successfully build you may need to install the following packages:
* GNU Autotools (https://www.gnu.org/software/autoconf/ http://www.gnu.org/software/automake/)
* Intltool >= 0.40.0 (http://freedesktop.org/wiki/Software/intltool/)
* GTK+ >= 3.22.0 (http://www.gtk.org)
with appropriate *-dev* packages depending on your distro

Additionally, you can build yad with the following libraries:
* Webkit - for supporting HTML dialog (http://webkitgtk.org)
* GtkSourceView - for enabling syntax highlighting in text-info dialog (https://wiki.gnome.org/Projects/GtkSourceView)
* GSpell - for support spell checking in text fields (https://wiki.gnome.org/Projects/gspell)

In standalone build (configure option --enable-standalone) some defaults can be redefined with the following defines

BORDERS - set the default border width around dialog. Default is 5  
REMAIN - if defined, timeout indicator will show the remaining time  
COMBO_EDIT - if defined, combo-box in entry dialog will be always editable  
TERM_CMD - string with terminal command. Default is "xterm -e '%s'"  
OPEN_CMD - string with open command. Default is "xdg-open '%s'"  
DATE_FMT - string with date output format. Default is "%x". See strftime(3) for details  
URI_COLOR - color for URIs in text-info dialog. Default is blue  
MARK1_COLOR - color for first type of text marks in text-info dialog. Default is lightgreen
MARK2_COLOR - color for second type of text marks in text-info dialog. Default is pink
MAX_TABS - set the number of tabs for tabbed dialog. Default is 100

Defines can be added througs CFLAGS environment variable
