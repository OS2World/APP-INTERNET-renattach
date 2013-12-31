#define MAX_ZIPBYTES	0x03000000	/* Arbitrary max reasonable size, detecting corrupt */

struct zip_info
{
	int offset;
	int skipping;		/* Flag */
	unsigned int skipcount;	/* Counter for bytes skipped after filename */
	unsigned int namelen;	/* Length of filename */
	char filename[512];
};

void trim_leading(char* buf);
void trim_trailing(char* buf);
void trim_trailch(char* buf, char ch1, char ch2, char ch3);
void expand_list(char** buffer, const char* addition, const char* delim);
char* stristr(const char* haystack, const char* needle);
unsigned char hex2int(char* hexform);
void decode_hex(char* text, char hexflag, int underscore, char* optdest);
int base64_decode_line(const char* input, char* dest);
void base64_encode_line(const char* input, int insize, char* output);
void init_zip(struct zip_info* zip);
char* unzip_filename(char* databuf, int databuf_len, struct zip_info* zip);
