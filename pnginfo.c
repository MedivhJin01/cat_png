#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "lab_png.h"
#include "crc.c"


int main(int argc, char **argv)
{
	FILE* fp;
	U8 png_buf[3];
	memset(png_buf, 0, 3);
	fp = fopen(argv[1], "rb");
	fseek(fp, 1, SEEK_SET);
	
	int count = fread(&png_buf, 1, 3, fp);
	int res;
	res = is_png(png_buf, 3);
	if(res == 1)
	{
		data_IHDR_p png_IHDR = malloc(sizeof(data_IHDR_p));
		memset(png_IHDR, 0, sizeof(data_IHDR_p));

		get_png_data_IHDR(png_IHDR, fp, 16);
		int height;
		int width;

		height = get_png_height(png_IHDR);
		width = get_png_width(png_IHDR);
		
		printf("%s: %d x %d\n", argv[1], width, height);
		
		/*Sept 21 Stars here:*/
        U32 length;
        fseek(fp, 33, SEEK_SET);
        fread(&length, sizeof(U32), 1, fp);
        length = ntohl(length);

        int td_length;
        td_length = length + 4;
        /*printf("length: %d\n", length);*/

        unsigned char *p_data = malloc(sizeof(unsigned char)*td_length);
        fseek(fp, 37, SEEK_SET);
        fread(p_data, sizeof(unsigned char),  td_length, fp);
        U32 read_crc;
        fseek(fp, 41 + length, SEEK_SET);
        fread(&read_crc, sizeof(U32), 1, fp);
        read_crc = ntohl(read_crc);
        /*printf("read_crc: %x\n", ntohl(read_crc));*/
        
        U32 cal_crc;
        cal_crc = crc(p_data, td_length);

        if(cal_crc != read_crc)
        {
            printf("IDAT chunk CRC error: computed %x, expected %x\n", cal_crc, read_crc);

        }
		free(png_IHDR);
        free(p_data);
	}
	else printf("%s: Not a PNG file\n", argv[1]);

	fclose(fp);
	return 0;
}

//int is_png(U8* buf, size_t n)
//{
//	U8 head[n];
//	head[0] = 0x50;
//	head[1] = 0x4e;
//	head[2] = 0x47;
//	int res;
//	res = memcmp(head, buf, 3);
//
//	if(res == 0)
//	{
//		return 1;
//	}
//
//	else return 0;
//
//}

int get_png_height(struct data_IHDR *buf)
{
	int height = ntohl(buf->height);
	return height;
}

int get_png_width(struct data_IHDR *buf)
{
	int width = ntohl(buf->width);
	return width;
}

int get_png_data_IHDR(struct data_IHDR *out, FILE *fp, long offset)
{
    fseek(fp, offset, SEEK_SET);
    fread(out, sizeof(*out), 1, fp);
	return 0;
}

int get_png_data_IDAT(struct chunk *out, FILE *fp, long offset)
{
	fseek(fp, offset, SEEK_SET);
	fread(out, sizeof(*out), 1, fp);
	return 0;
}

U32 get_crc(struct chunk *IDAT)
{
	return IDAT->crc;
}

/*Sept21 Sarts here*/
int get_chunk_data(chunk_p IDAT, FILE *fp)
{
	fseek(fp, 37, SEEK_SET);
	fread(IDAT, sizeof(*IDAT), 1, fp);	
	return 0;
}

//
//00000000: 8950 4e47 0d0a 1a0a 0000 000d 4948 4452  .PNG........IHDR
//00000010: 0000 0010 0000 0010 0806 0000 001f f3ff  ................
//00000020: 6100 0000 2449 4441 5478 9c63 fccf c0d0  a...$IDATx.c....
//00000030: c080 0730 fec7 27cb c0c0 845f 9a30 1835  ...0..'...._.0.5
//00000040: 60d4 8051 0306 8b01 0044 9802 9fdc 5f7b  `..Q.....D...._{
//00000050: 8400 0000 0045 4e44 0a83 4ab4 c6         .....END..J..


//00 0000 0049 454e 44ae 4260 82


