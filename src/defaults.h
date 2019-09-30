#ifndef __YAD_DEFS_H__
#define __YAD_DEFS_H__

#ifndef BORDERS
#define BORDERS 5
#endif

#ifndef REMAIN
#define SHOW_REMAIN FALSE
#else
#define SHOW_REMAIN TRUE
#endif

#ifndef COMBO_EDIT
#define COMBO_ALWAYS_EDIT FALSE
#else
#define COMBO_ALWAYS_EDIT TRUE
#endif

#ifndef TERM_CMD
#define TERM_CMD "xterm -e '%s'"
#endif

#ifndef OPEN_CMD
#define OPEN_CMD "xdg-open '%s'"
#endif

#ifndef DATE_FMT
#define DATE_FMT "%x"
#endif

#ifndef URI_COLOR
#define URI_COLOR "blue"
#endif

#ifndef MAX_TABS
#define MAX_TABS 100
#endif

#endif /* __YAD_DEFS_H__ */
