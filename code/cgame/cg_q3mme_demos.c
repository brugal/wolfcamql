/*
===========================================================================
Copyright (C) 2009 Sjoerd van der Berg ( harekiet @ gmail.com )

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "cg_q3mme_demos.h"
#include "cg_syscalls.h"

demoMain_t demo;

void CG_DemosAddLog (const char *fmt, ...)
{
	va_list args;
	char text[MAX_PRINT_MSG];

	va_start(args, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, args);
	va_end(args);

	trap_Print(va("^6q3mme: ^7%s\n", text));
}
