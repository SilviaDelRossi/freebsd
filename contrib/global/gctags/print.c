/*
 * Copyright (c) 1987, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)print.c	8.3 (Berkeley) 4/2/94";
#endif /* LIBC_SCCS and not lint */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "die.h"
#include "ctags.h"

/*
 * getline --
 *	get the line the token of interest occurred on,
 *	prepare it for printing.
 */
void
getline()
{
	long	saveftell;
	int	c;
	int	cnt;
	char	*cp;

	saveftell = ftell(inf);
	(void)fseek(inf, lineftell, SEEK_SET);
	if (xflag)
		for (cp = lbuf; GETC(!=, EOF) && c != '\n'; *cp++ = c)
			continue;
	/*
	 * do all processing here, so we don't step through the
	 * line more than once; means you don't call this routine
	 * unless you're sure you've got a keeper.
	 */
	else for (cnt = 0, cp = lbuf; GETC(!=, EOF) && cnt < ENDLINE; ++cnt) {
		if (c == '\\') {		/* backslashes */
			if (cnt > ENDLINE - 2)
				break;
			*cp++ = '\\'; *cp++ = '\\';
			++cnt;
		}
		else if (c == (int)searchar) {	/* search character */
			if (cnt > ENDLINE - 2)
				break;
			*cp++ = '\\'; *cp++ = c;
			++cnt;
		}
		else if (c == '\n') {	/* end of keep */
			*cp++ = '$';		/* can find whole line */
			break;
		}
		else
			*cp++ = c;
	}
	*cp = EOS;
	(void)fseek(inf, saveftell, SEEK_SET);
}
#ifdef GLOBAL
void
compact_print(entry, lno, file)
char	*entry;
int	lno;
char	*file;
{
	static	int first = 1;
	static	char p_entry[128];
	static	char p_file[1024];
	static	int  p_lno;
	static	char *buf;
	static	int bufsize = 512;
	static	char *p;

	if (first) {
		if (!(buf = (char *)malloc(bufsize)))
			die("short of memory.");
		buf[0] = 0;
		p = buf;
		first = 0;
	}
	if (strcmp(p_entry, entry) || strcmp(p_file, file)) {
		if (buf[0])
			printf("%s\n", buf);
		if (!entry[0])			/* flush */
			return;
		strcpy(p_entry, entry);
		strcpy(p_file, file);
		p_lno = lno;
		buf[0] = 0;
		sprintf(buf, "%s %s %d", entry, file, lno);
		p = buf;
		p += strlen(p);
	} else {
		if (p_lno > lno)
			die("impossible!");
		if (p_lno < lno) {
			if (buf + bufsize < p + 10) {
				int offset = p - buf;
				bufsize *= 2;
				if (!(buf = (char *)realloc(buf, bufsize)))
					die("short of memory.");
				p = buf + offset;
			}
			sprintf(p, ",%d", lno);
			p += strlen(p);
			p_lno = lno;
		}
	}
}
#endif
/*
 * put_entries --
 *	write out the tags
 */
void
put_entries(node)
	NODE	*node;
{

	if (node->left)
		put_entries(node->left);
	if (vflag)
		printf("%s %s %d\n",
		    node->entry, node->file, (node->lno + 63) / 64);
#ifdef GLOBAL
	else if (xflag && cflag)
		compact_print(node->entry, node->lno, node->file);
#endif
	else if (xflag)
#ifdef GLOBAL
		/* separate 'entry' and 'lno' */
		if (strlen(node->entry) >= 16 && node->lno >= 1000)
			printf("%-16s %4d %-16s %s\n",
			    node->entry, node->lno, node->file, node->pat);
		else /* for compatibility */
#endif
		printf("%-16s%4d %-16s %s\n",
		    node->entry, node->lno, node->file, node->pat);
	else
		fprintf(outf, "%s\t%s\t%c^%s%c\n",
		    node->entry, node->file, searchar, node->pat, searchar);
	if (node->right)
		put_entries(node->right);
}
