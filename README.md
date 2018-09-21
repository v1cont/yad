# Yet Another Dialog
====================

Program allows you to display GTK+ dialog boxes from command line or 
shell scripts. YAD depends on GTK+ only. Minimal GTK+ version is 2.16.0

This software is licensed under the GPL v.3

Project homepage: https://sourceforge.net/projects/yad-dialog/  
Example usage: https://sourceforge.net/p/yad-dialog/wiki/browse_pages/  
Mailing list: http://groups.google.com/group/yad-common

Some miscellaneous stuff can be found in data directory including notify-send script
and simple zenity-compatible wrapper 

# Building git version
======================

Get git version with command

git clone https://github.com/v1cont/yad.git yad-dialog-code

Before run the standard ./configure && make && make install procedure
you need to generate build scripts. This can be done by running command

autoreconf -ivf && intltoolize

For successfully build you may need to install the following packages:
* GNU Autotools (https://www.gnu.org/software/autoconf/ http://www.gnu.org/software/automake/)
* Intltool >= 0.40.0 (http://freedesktop.org/wiki/Software/intltool/)
* GTK+ >= 2.16.0 (http://www.gtk.org)
with appropriate *-dev* packages depending on your distro

Additionally, you can build yad with the following libraries:
* Webkit - for supporting HTML dialog (http://webkitgtk.org)
* GtkSourceView - for enabling syntax highlighting in text-info dialog (https://wiki.gnome.org/Projects/GtkSourceView)
* GtkSpell3 - for support spell checking in text fields (http://gtkspell.sourceforge.net/)

Yad can be built against both 2 and 3 versions of GTK+ library. Preferred build with version 2
