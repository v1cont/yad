/*
 * This file is part of YAD.
 *
 * YAD is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * YAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with YAD. If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2020-2021, Victor Ananjevsky <ananasik@gmail.com>
 */

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

#ifndef MARK1_COLOR
#define MARK1_COLOR "lightgreen"
#endif

#ifndef MARK2_COLOR
#define MARK2_COLOR "pink"
#endif

#endif /* __YAD_DEFS_H__ */
