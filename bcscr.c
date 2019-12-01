/************************************************************************
A quick and simple way to create a source code report.  Inputs are two 
directories for counting and comparing.  The output is the total files 
and lines in each directory, and the line difference in number of changed 
lines, number of added lines, and number of deleted lines.

Author:  James Henrie (jbhenrie@users.sourceforge.net)

Copyright (c) 2010 James Henrie

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Build:
	$ gcc bcscr.c -o bcscr

Use & output:
	$ ./bcscr dir1 dir2
	dir1: files=65220 lines=25380461
	dir2: files=59923 lines=22935318
	lines: changed=81723 added=264722 deleted=167850

Project website: 
	http://bcscr.sourceforge.net
	svn co https://bcscr.svn.sourceforge.net/svnroot/bcscr bcscr
************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define DIFFARGS			"-rdbNH"
#define FINDARGS			"-type f"
#define WCARGS				"-l"
#define TEMPFILENAME		".bcscr_tempfile"
#define MIN(a,b)			((a)<(b)?(a):(b))

#define ERR(fmt, args...) printf("%s %d " fmt, __FUNCTION__, __LINE__, ## args);

void usage(char* filename)
{
	printf("\n%s dir1 dir2\n\n", filename);
}

int cmd_diff(char* args, char* dir1, char* dir2, char* outfilename)
{
	int ret;
	char command[255];

	sprintf(command, "diff %s %s %s > %s\n", 
		args, dir1, dir2, outfilename);
	ret = system(command);
	if (ret == -1)
	{
		ERR("system(diff) failed\n");
		return -1;
	}

	return 0;
}

int cmd_find(char* dir, char* outfilename)
{
	int ret;
	char command[255];

	sprintf(command, "find %s %s -exec wc %s {} \\; > %s\n", 
		dir, FINDARGS, WCARGS, outfilename);
	ret = system(command);
	if (ret == -1 || !WIFEXITED(ret) || WEXITSTATUS(ret) != 0)
	{
		ERR("system(find) failed with %d %d %d\n", 
			ret, WIFEXITED(ret), WEXITSTATUS(ret));
		return -1;
	}

	return 0;
}

int cmd_rm(char* filename)
{
	int ret;
	char command[255];

	sprintf(command, "rm %s\n", filename);
	ret = system(command);
	if (ret == -1 || !WIFEXITED(ret) || WEXITSTATUS(ret) != 0)
	{
		ERR("system(rm) failed with %d %d %d\n", 
			ret, WIFEXITED(ret), WEXITSTATUS(ret));
		return -1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	int gp;
	int gm; 
	int lines;
	int ret = 0;
	int dir1_files;
	int dir2_files;
	int dir1_lines;
	int dir2_lines;
	int buf_bytes;
	int add_lines;
	int del_lines;
	int change_lines;
	char* buf = NULL;
	FILE* infile;

	if (argc != 3)
	{
		usage(argv[0]);
		return -1;
	}

	infile = fopen(TEMPFILENAME, "w+");
	if (!infile)
	{
		ERR("failed to open temp file %d (%s)\n", 
			-errno, strerror(errno));
		ret = -1;
		goto out;
	}

	if (cmd_find(argv[1], TEMPFILENAME))
	{
		ERR("find failed\n");
		ret = -1;
		goto out;
	}

	dir1_files = 0;
	dir1_lines = 0;
	fseek(infile, 0, SEEK_SET);
	while(!feof(infile) && fscanf(infile, "%d", &lines) != EOF && 
		getline(&buf, &buf_bytes, infile) != -1)
	{
		dir1_files++;
		dir1_lines += lines;
	}

	if (cmd_find(argv[2], TEMPFILENAME))
	{
		ERR("find failed\n");
		ret = -1;
		goto out;
	}

	dir2_files = 0;
	dir2_lines = 0;
	fseek(infile, 0, SEEK_SET);
	while(!feof(infile) && fscanf(infile, "%d", &lines) != EOF && 
		getline(&buf, &buf_bytes, infile) != -1)
	{
		dir2_files++;
		dir2_lines += lines;
	}

	if (cmd_diff(DIFFARGS, argv[1], argv[2], TEMPFILENAME))
	{
		ERR("diff failed\n");
		ret = -1;
		goto out;
	}

	gp = 0;
	gm = 0;
	add_lines = 0;
	del_lines = 0;
	change_lines = 0;
	fseek(infile, 0, SEEK_SET);
	while(!feof(infile) && getline(&buf, &buf_bytes, infile) != -1)
	{
		switch(buf[0])
		{
			case '<':
				gm++;
				break;

			case '>':
				gp++;
				break;

			case '-':
				break;

			default:
				change_lines += MIN(gp, gm);
				if (gp > gm)
				{
					add_lines += gp - gm;
				}
				else
				{
					del_lines += gm - gp;
				}
				gp = 0;
				gm = 0;
				break;
		}
	}
	printf("%s: files=%d lines=%d\n", argv[1], dir1_files, dir1_lines);
	printf("%s: files=%d lines=%d\n", argv[2], dir2_files, dir2_lines);
	printf("lines: changed=%d added=%d deleted=%d\n", 
		change_lines, add_lines, del_lines);

out:
	free(buf);
	fclose(infile);
	cmd_rm(TEMPFILENAME);
	return ret;
}
