Yet Another Dialog
====================

NB:

Well, sorry for silence but i'm too busy right now (fucking war,
fucking russians). After all i'm still alive

Just want to say - YAD is not an abandoned project.  Because of
external circumstances i cannot maintain it very actively, but i lurk
for all of yours reports and proposals and keeps it in my mind

Thanks for understanding and patience

=========

Program allows you to display GTK+ dialog boxes from command line or 
shell scripts. YAD depends on GTK+ only. Minimal GTK+ version is 3.22.0

This software is licensed under the GPL v.3

Project homepage: https://github.com/v1cont/yad  
Complex examples: https://github.com/v1cont/yad/wiki/YAD-Examples  
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

autoreconf -ivf

You must manually run gtk-update-icon-cache after installation.

For successfully build you may need to install the following packages:
* GNU Autotools (https://www.gnu.org/software/autoconf/ http://www.gnu.org/software/automake/)
* gettext >= 0.19.7 (https://www.gnu.org/software/gettext/)
* GTK+ >= 3.22.0 (http://www.gtk.org)
with appropriate *-dev* packages depending on your distro

Additionally, you can build yad with the following libraries:
* Webkit - for supporting HTML dialog (http://webkitgtk.org)
* GtkSourceView - for enabling syntax highlighting in text-info dialog (https://wiki.gnome.org/Projects/GtkSourceView)
* GSpell - for support spell checking in text fields (https://wiki.gnome.org/Projects/gspell)
* Libappindicator - for support appindicator extension (https://ayatana-indicators.org/)
