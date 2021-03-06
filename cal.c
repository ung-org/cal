/*
 * UNG's Not GNU
 *
 * Copyright (c) 2011-2019, Jakob Kaivo <jkk@ung.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define _XOPEN_SOURCE 700
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MONTHWIDTH	(20)		/* 7 two-digit days + spacing */
#define MONTHHEIGHT	(8)		/* labels + max 6 weeks */
#define COLUMNS		(3)		/* arbitrary, but looks decent */
#define COLUMNSEP	"  "		/* arbitrary, but looks decent */

/* FIXME: Sep 1752 cuts off after the 27th */

static char displaymonth[COLUMNS][MONTHHEIGHT][MONTHWIDTH+2];

static int daysin(int year, int month)
{
	int mdays[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	if (month == 1 && ((year % 4 == 0 && year % 400 == 0)
		|| (year % 4 == 0 && year % 100 != 0))) {
			return 29;
	}
	return mdays[month];
}

static int dow(int y, int m, int d)
{
	int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
	int shift = 0;

	/* Julian dates prior to 1752-09-14 (1752-09-02 is the day before) */
	if (y < 1752 || (y == 1752 && m < 9) || (y == 1752 && m == 9 && d < 14)) {
		shift = 3;
	}

	y -= m < 3;
	return ((y + (y / 4) - (y / 100) + (y / 400) + t[m - 1] + d) - shift) % 7;
}

static char *center(char *s, size_t n)
{
	char centered[n+1];
	size_t len = strlen(s);
	size_t lpad = (n - len) / 2;
	memset(centered, ' ', n);
	strcpy(centered + lpad, s);
	centered[lpad + len] = ' ';
	centered[n] = '\0';
	return strcpy(s, centered);
}

static int cal_week(char *buf, int year, int month, int week)
{
	int first = dow(year, month, 1);
	int start = (week * 7) - first;
	int chars = 0;

	if (start < 0) {
		start = 0;
	}

	int days = daysin(year, month - 1);

	if (week == 0) {
		for (int day = 0; day < first; day++) {
			chars += sprintf(buf + chars, "   ");
		}
	}

	for (int day = start + 1; day < start + 8; day++) {
		int shift = 0;
		/* Julian dates prior to 1752-09-14 (1752-09-02 is the day before) */
		if (year == 1752 && month == 9 && day > 2) {
			shift = 11;
		}

		if (day + shift > days) {
			return chars;
		}

		chars += snprintf(buf + chars, MONTHWIDTH+1-chars, "%2d%s", day + shift, dow(year, month, day) == 6 ? "" : " ");

		if (dow(year, month, day) == 6) {
			return chars;
		}
	}

	return 0;
}

void genmonth(struct tm *tm, int withyear)
{
	strftime(displaymonth[tm->tm_mon % COLUMNS][0], MONTHWIDTH,
		withyear ? "%B %Y" : "%B", tm);
	center(displaymonth[tm->tm_mon % COLUMNS][0], MONTHWIDTH);

	char *header = "Su Mo Tu We Th Fr Sa";
	strcpy(displaymonth[tm->tm_mon % COLUMNS][1], header);

	for (int i = 0; i < MONTHHEIGHT-2; i++) {
		memset(displaymonth[tm->tm_mon % COLUMNS][i+2], ' ',
			MONTHWIDTH);
		cal_week(displaymonth[tm->tm_mon % COLUMNS][i+2],
			tm->tm_year + 1900, tm->tm_mon + 1, i);
		displaymonth[tm->tm_mon % COLUMNS][i+2][strlen(displaymonth[tm->tm_mon % COLUMNS][i+2])] = ' ';
		displaymonth[tm->tm_mon % COLUMNS][i+2][MONTHWIDTH] = '\0';
	}
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");

	while (getopt(argc, argv, "") != -1) {
		return 1;
	}

	time_t now = time(NULL);
	struct tm *tm = localtime(&now);
	tm->tm_mday = 1;

	if (argc > optind + 2) {
		fprintf(stderr, "cal: too many operands\n");
		return 1;
	}

	if (argc > optind) {
		char *year = argv[argc - 1];
		tm->tm_year = atoi(year) - 1900;
		if (tm->tm_year > 9999-1900 || tm->tm_year < 1-1900) {
			fprintf(stderr, "cal: invalid year %s\n", year);
			return 1;
		}
	}

	if (argc > optind + 1) {
		char *month = argv[optind];
		tm->tm_mon = atoi(month) - 1;
		if (tm->tm_mon > 11 || tm->tm_mon < 0) {
			fprintf(stderr, "cal: invalid month %s\n", month);
			return 1;
		}
	}

	if (argc != optind + 1) {
		genmonth(tm, 1);
		for (int i = 0; i < MONTHHEIGHT; i++) {
			puts(displaymonth[tm->tm_mon % COLUMNS][i]);
		}
		return 0;
	}

	char year[MONTHWIDTH * COLUMNS + (sizeof(COLUMNSEP) - 1) * (COLUMNS - 1)] = {0};
	strftime(year, sizeof(year), "%Y", tm);
	printf("%s\n\n", center(year, sizeof(year)));

	tm->tm_mon = 0;
	while (tm->tm_mon < 12) {
		for (int row = 0; row < MONTHHEIGHT; row++) {
			for (int col = 0; col < COLUMNS; col++) {
				if (row == 0) {
					genmonth(tm, 0);
					tm->tm_mon++;
				}
				printf("%s%s", displaymonth[col][row],
					col == 2 ? "\n" : COLUMNSEP);
			}
		}
	}

	return 0;
}
