#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define HISTCAP 32
#define HISTLINE 128

static char *histfile;
static char history[HISTCAP][HISTLINE];
static int histpos = -1;
static int histtop = -1;
static char hold[BUFSIZ] = {0};

void
loadhistory(void)
{
	FILE *fp = NULL;
	int i = 0;

	if (!histfile) {
		return;
	}

	fp = fopen(histfile, "r");
	if (!fp) {
		for (i = histtop+1; i < HISTCAP; i++) {
			history[i][0] = '\0';
		}
		histpos++;
		return;
	}

	while (histpos < HISTCAP) {
		if (fgets(history[histpos+1], HISTLINE, fp) == NULL) {
			if (ferror(fp)) {
				die("failed to read history");
			}
			break;
		}
		strtok(history[histpos+1], "\n");
		if (history[histpos+1][0] == '\n') break;
		histpos++;
	}

	if (fclose(fp)) {
		die("failed to close file %s", histfile);
	}
	histtop = histpos;
	for (i = histtop+1; i < HISTCAP; i++) {
		history[i][0] = '\0';
	}
	histpos++;
}

void
navhistory(int dir)
{
	char *p = NULL;
	size_t len = 0;

	if (histtop < 0) return;
	if (!histpos && dir < 0) return;
	if (histpos == HISTCAP || history[histpos][0] == '\0') {
		if (dir > 0) return;
		else strcpy(hold, text);
	}

	switch(dir) {
	case 1:
		histpos++;
		if (histpos < HISTCAP && history[histpos][0] != '\0') {
			p = history[histpos];
		} else  {
			p = hold;
		}
		break;
	case -1:
		p = history[--histpos];
		break;
	}

	len = MIN(strlen(p), BUFSIZ - 1);
	strcpy(text, p);
	cursor = len;
	match();
}

void
addhistory(char *input)
{
	int dup;
	int i;

	if (
		!histfile || !input || !HISTCAP || !input[0] ||
		strlen(input) > HISTLINE
	) {
		return;
	}
	strtok(input, "\n");

	for (dup = histtop; dup >= 0; dup--) if (!strcmp(input, history[dup])) break;

	if (dup < 0 && histtop+1 == HISTCAP) dup = 0;

	if (dup < 0) {
		strcpy(history[++histtop], input);
	} else {
		for (i = dup; i < histtop; i++) {
			strcpy(history[i], history[i+1]);
		}
		strcpy(history[histtop], input);
	}
}

void
savehistory(void)
{
	unsigned int i;
	FILE *fp;

	if (!histfile || 0 == HISTCAP) return;

	fp = fopen(histfile, "w");
	if (!fp) {
		die("failed to open %s", histfile);
	}

	for (i = 0; i <= histtop; i++) {
		if (0 >= fprintf(fp, "%s\n", history[i])) {
			die("failed to write to %s", histfile);
		}
	}

	if (fclose(fp)) {
		die("failed to close file %s", histfile);
	}
}
