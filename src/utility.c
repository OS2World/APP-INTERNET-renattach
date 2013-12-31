/*
 renattach 1.2.2 - Filter that renames/deletes dangerous email attachments
 Copyright (C) 2003, 2004  Jem E. Berkes <jberkes@pc-tools.net>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "utility.h"


/* Used by base64 routines */
const static char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


/*
	Trim leading whitespace from a line
*/
void trim_leading(char* buf)
{
	int i, len = strlen(buf);
	char *tmpbuf = calloc(len+1, 1);
	for (i=0; i<len; i++)
	{
		if (buf[i]!=' ' && buf[i]!='\t')
			break;
	}
	strcpy(tmpbuf, buf+i);
	strcpy(buf, tmpbuf);
	free(tmpbuf);
}


/*
	Trims trailing whitespace from a line
*/
void trim_trailing(char* buf)
{
	char* end = buf+strlen(buf)-1;
	while (end>=buf && (*end==' ' || *end=='\t'))
	{
		*end = 0;
		end--;
	}
}


/*
	Trims specified trailing character from a line
*/
void trim_trailch(char* buf, char ch1, char ch2, char ch3)
{
	char* end = buf+strlen(buf)-1;
	while (end>=buf && (*end==ch1 || *end==ch2 || *end==ch3))
	{
		*end = 0;
		end--;
	}
}


/*
	Expand the specified dynamic list with the additional text
	If adding to existing, the new addition is delimited by the given string
*/
void expand_list(char** buffer, const char* addition, const char* delim)
{
	char* newbuf;
	if (*buffer)
	{
		newbuf = malloc(strlen(*buffer) + strlen(delim) + strlen(addition) + 1);
		strcpy(newbuf, *buffer);
		strcat(newbuf, delim);
	}
	else
		newbuf = calloc(strlen(addition) + 1, 1);
	strcat(newbuf, addition);
	if (*buffer)
		free(*buffer);
	*buffer = newbuf;
}


/*
	Case-insensitive version of strstr
	Searches for needle within haystack, returning pointer
	to start of the matching characters in haystack.
*/
char* stristr(const char* haystack, const char* needle)
{
	const char* needlepoint = needle;
	const char* matchpoint = NULL;

	if (!haystack || !needle)	/* check for bad input */
		return NULL;
	else if (*needle == '\0')	/* strstr compliance */
		return (char*)haystack;

	while (*haystack != '\0')
	{
		if (tolower(*needlepoint) == tolower(*haystack))
		{
			if (!matchpoint)
				matchpoint = haystack;
			needlepoint++;
			if (*needlepoint == '\0')
				return (char*)matchpoint;
		}
		else if (matchpoint)
		{	/* reset the search */
			needlepoint = needle;
			haystack = matchpoint; /* cure overlap problem */
			matchpoint = NULL;
		}
		haystack++;
	}
	return NULL;
}


/*
	Return integer (byte) equivalent of hexadecimal octet from ASCII form
*/
unsigned char hex2int(char* hexform)
{
	int pos;
	unsigned char value = 0;

	for (pos=0; pos<=1; pos++)
	{
		unsigned char nibble = (unsigned char)hexform[pos];
		if ((nibble >= '0') && (nibble <= '9'))
			nibble -= '0';
		else if ((nibble >= 'a') && (nibble <= 'f'))
			nibble -= ('a' - 10);
		else if ((nibble >= 'A') && (nibble <= 'F'))
			nibble -= ('A' - 10);
		else
			return value;
		value = (value << 4) | nibble;
	}
	return value;
}


/*
	Decode quoted printable (or URL-style) encoding where 8-bit characters
	are represented by hexadecimal octet. e.g. Hello%20world%2E

	Specify the hexflag character
	Set underscore=1 if underscores should be converted to spaces (RFC 2047)

	optdest is optional; if NULL, the input text is replaced
*/
void decode_hex(char* text, char hexflag, int underscore, char* optdest)
{
	int pos, inlen = strlen(text);
	char* tmpbuf = calloc(inlen+1, 1);
	for (pos=0; pos < inlen; pos++)
	{
		if (underscore && (text[pos]=='_'))
			strcat(tmpbuf, " ");
		else if ((text[pos]==hexflag) && (pos+2<inlen))
		{
			pos++;
			if (isxdigit((int)text[pos]) && isxdigit((int)text[pos+1]))
				sprintf(tmpbuf+strlen(tmpbuf), "%c", hex2int(text+pos++));
		}
		else
			sprintf(tmpbuf+strlen(tmpbuf), "%c", text[pos]);
	}
	if (optdest)
		strcpy(optdest, tmpbuf);	/* optdest better be large enough */
	else
		strcpy(text, tmpbuf);	/* decoded version can never be longer than original */
	free(tmpbuf);
}


/*
	Returns number of bytes written to dest, if this was valid base64 encoded data
	Returns 0 if invalid format
	Can decode non-text. Caller should completely clear dest
*/
int base64_decode_line(const char* input, char* dest)
{
	char outbytes[3];
	int len = strlen(input);	/* input is plain text */
	const char* endpoint = input + len;
	int outpos = 0;		/* required for binary output */

	if (len % 4 != 0)
		return 0;	/* quit, invalid format */
	while (input < endpoint)
	{
		int i, j, towrite=3;
		for (i=2; i<4; i++)	/* count deductions */
			if (input[i] == '=') towrite--;
		for (j=0; j<towrite; j++)
		{
			char *lookup1 = strchr(alphabet, input[j]);
			char *lookup2 = strchr(alphabet, input[j+1]);
			if (!lookup1 || !lookup2)
				return 0;	/* invalid situation */
			outbytes[j] = (((lookup1-alphabet) << (2+j*2)) | ((lookup2 - alphabet) >> (4-2*j)));
		}
		memcpy(dest+outpos, outbytes, towrite);
		outpos += towrite;
		input += 4;
	}
	return outpos;
}


/*
	base64 encoded the input data, of size insize
	output buffer must hold: 1 + (1+insize/3)*4
*/
void base64_encode_line(const char* input, int insize, char* output)
{
	int i;
	unsigned char inthree[3];
	unsigned char outfour[4];

	for (i=0; i < (insize/3); i++)
	{
		memcpy(inthree, input+(3*i), 3);
		outfour[0] = inthree[0]>>2;
		outfour[1] = ((inthree[0]<<4) & 0x30) | (inthree[1]>>4);
		outfour[2] = ((inthree[1]<<2) & 0x3F) | (inthree[2]>>6);
		outfour[3] = inthree[2] & 0x3F;
		outfour[0] = alphabet[outfour[0]];
		outfour[1] = alphabet[outfour[1]];
		outfour[2] = alphabet[outfour[2]];
		outfour[3] = alphabet[outfour[3]];
		memcpy(output+(4*i), outfour, 4);
	}
	if (insize % 3 == 1)
	{
		memcpy(inthree, input+(3*i), 1);
		outfour[0] = inthree[0] >> 2;
		outfour[1] = (inthree[0]<<4) & 0x30;
		outfour[0] = alphabet[outfour[0]];
		outfour[1] = alphabet[outfour[1]];
		outfour[2] = '=';
		outfour[3] = '=';
		memcpy(output+(4*i), outfour, 4);
		i++;
	}
	else if (insize % 3 == 2)
	{
		memcpy(inthree, input+(3*i), 2);
		outfour[0] = inthree[0] >> 2;
		outfour[1] = ((inthree[0]<<4) & 0x30) | (inthree[1]>>4);
		outfour[2] = (inthree[1]<<2) & 0x3F;
		outfour[0] = alphabet[outfour[0]];
		outfour[1] = alphabet[outfour[1]];
		outfour[2] = alphabet[outfour[2]];
		outfour[3] = '=';
		memcpy(output+(4*i), outfour, 4);
		i++;
	}
	memcpy(output+(4*i), "\0", 1);
}


/*
	You must initialize the zip_info structure before first use
*/
void init_zip(struct zip_info* zip)
{
	memset(zip, 0, sizeof(struct zip_info));
	zip->offset = -1;
}


/*
	Search for and extract filenames from ZIP archives.
	databuf is the input chunk of binary ZIP data, of databuf_len
	zip_info is the context, and must have been initialized

	Returns:
	NULL	No filename found in this data block
	ptr	Filename found. Repeat call with this pointer to continue
		processing this data block after you grab the filename.
*/
char* unzip_filename(char* databuf, int databuf_len, struct zip_info* zip)
{
	char* databuf_pos;		/* Position within databuf */
	char* databuf_end;		/* Byte past end of databuf */

	/* Sanity checking */
	if (!databuf || !zip)
		return NULL;
	if (databuf_len < 1)
		return NULL;

	databuf_pos = databuf;
	databuf_end = databuf + databuf_len;

	while (databuf_pos < databuf_end)
	{
		/* Maybe we're skipping compresed data until next file */
		if (zip->skipping)
		{
			if (zip->skipcount == 0)
				init_zip(zip);	/* Clears skipping flag too */
			else
			{
				databuf_pos++;
				zip->skipcount--;
			}
			continue;
		}

		if (zip->offset < 0)	/* haven't yet found first ID byte */
		{
			void* id0 = memchr(databuf_pos, 0x50, databuf_end-databuf_pos);
			if (id0)
			{
				zip->offset = 0;	/* found 0x50, offset 0 ok */
				databuf_pos = (char*)id0+1;
				continue;
			}
			else
				return NULL;	/* no hope in this particular data block */
		}

		if (zip->offset < 3)
		{
			char needid = 0;
			switch (zip->offset)
			{
				case 0:
					needid = 0x4b;
					break;
				case 1:
					needid = 0x03;
					break;
				case 2:
					needid = 0x04;
					break;
			}

			if (*databuf_pos == needid)
				zip->offset++;
			else
				zip->offset = -1;
			databuf_pos++;
			continue;
		}

		zip->offset++;
		if (zip->offset == 0x12)
			zip->skipcount = (unsigned char)*databuf_pos;
		else if (zip->offset == 0x13)
			zip->skipcount |= ((unsigned char)*databuf_pos) << 8;
		else if (zip->offset == 0x14)
			zip->skipcount |= ((unsigned char)*databuf_pos) << 16;
		else if (zip->offset == 0x15)
			zip->skipcount |= ((unsigned char)*databuf_pos) << 24;
		else if (zip->offset == 0x1a)
			zip->namelen = (unsigned char)*databuf_pos;
		else if (zip->offset == 0x1b)
		{
			zip->namelen |= ((unsigned char)*databuf_pos) << 8;
			/* Bounds checking */
			if ((zip->namelen + 1) > sizeof(zip->filename))
				zip->namelen = sizeof(zip->filename) - 1;
		}
		else if (zip->offset == 0x1c)
			zip->skipcount += (unsigned char)*databuf_pos;
		else if (zip->offset == 0x1d)
		{
			zip->skipcount += ((unsigned char)*databuf_pos) << 8;
			if (zip->skipcount > MAX_ZIPBYTES)	/* Exceeded reasonable size */
			{
				init_zip(zip);	/* Likely corrupt, reset state*/
				continue;
			}
		}
		else if (zip->offset >= 0x1e)
		{
			/* Transfer bytes of filename until end is reached */
			if ((zip->offset - 0x1e) >= zip->namelen)
			{
				/* Next time, we'll start skipping bytes */
				zip->skipping = 1;
				return databuf_pos;	/* Done */
			}
			zip->filename[zip->offset - 0x1e] = *databuf_pos;
		}
		databuf_pos++;
	}
	return NULL;
}
