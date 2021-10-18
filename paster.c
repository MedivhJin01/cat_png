#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <curl/curl.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <getopt.h>
#include "paster.h"
#include "crc.c"
#include "lab_png.h"
#include "zutil.h"
#include "zutil.c"
//#include "crc.h"

//int cURL_fetch_PDF(char* url, int *seq, unsigned char *buf, int *size);
void* cURL_fetch_PDF(void* para);
int cat_png(unsigned char** pngs, int png_num);
int fetch_done(int seq_arr[]);

struct thread_input{
    char *url;
//    char *array;
};

struct thread_ret{
    
};

unsigned char **buf;
int pic_seq[50];

int main(int argc, char** argv)
{
    // initialize the picture and thread to default value
    int opt;
    int pic_num = 1;
    int thread_num = 1;
    
    int seq;
	int size[50];
	int all_size = 0;
	int i;
    
    buf = malloc(50*sizeof(char*));
    for(i = 0; i < 50; i++)
    {
        buf[i] = malloc(10000*sizeof(char));
    }
    
    for(i = 0; i < 50; i++)
    {
        pic_seq[i] = 1;
    }
    
    while ((opt = getopt (argc, argv, "t:n:")) != -1) {
        switch (opt) {
            case 't':
                thread_num = strtoul(optarg, NULL, 10);
                break;
                
            case 'n':
                pic_num = strtoul(optarg, NULL, 10);
                break;
        }
    }
    pthread_t tid[thread_num];
    
    for (int k = 0; k < thread_num; k++) {
        char url[50];
        memset(url, 0, 50);
        sprintf(url, "http://ece252-%d.uwaterloo.ca:2520/image?img=%d", k % 3 + 1 , pic_num);
        pthread_create(&tid[k], NULL, cURL_fetch_PDF, url);
        //free(url);
    }
    for (int j = 0; j < thread_num; j++) {
        pthread_join(tid[j], NULL);
    }
    
    
//
//	char *temp = malloc(10000*sizeof(char));
//	int temp_size;
//	while(pic_count)
//	{
//		int res = cURL_fetch_PDF("http://ece252-1.uwaterloo.ca:2520/image?img=2", &seq, temp, &temp_size);
//		if(pic_seq[seq] != 0)
//		{
//			memcpy(buf[seq], temp, temp_size);
//			size[seq] = temp_size;
//			all_size += temp_size;
//			pic_seq[seq]--;
//			pic_count--;
//		}
//
//	}
	
	cat_png(buf, 50);
	
	for(i = 0; i < 50; i++)
	{
		free(buf[i]);
	}
	free(buf);
    return 0;
}

int cat_png(unsigned char** pngs, int png_num)
{
	unsigned int total_height = 0;

	unsigned int width;
	unsigned int height;
	unsigned int type;
	U64 IDAT_len = 0;
    
    unsigned char inf_data[png_num*6*(400*4+1)];
    memset(inf_data, 0, png_num*6*(400*4+1));
	unsigned char inf[6*(400*4+1)];
    
    U64 inf_len = 0;
    U64 total_inf_len = 0;
    unsigned int total_def_len = 0;
    
	int i;
	for(i = 0; i < png_num; i++)
	{
		memcpy(&width, pngs[i] + WIDTH_POS, 4);
		width = ntohl(width);
		if(width != 400)
		{
			printf("unexpected width\n");
			return 1;
		}

		memcpy(&height, pngs[i] + HEIGHT_POS, 4);
		total_height += ntohl(height);

		memcpy(&IDAT_len, pngs[i] + IDAT_LENGTH_POS, 4);
        IDAT_len = ntohl(IDAT_len);
		
        total_def_len += IDAT_len;
        memcpy(&type, pngs[i] + IDAT_TYPE_POS, 4);
		
        U8 data[IDAT_len];
        
        memset(data, 0, IDAT_len*sizeof(unsigned char));
		memcpy(data, pngs[i] + IDAT_DATA_POS, IDAT_len);
        
        int res = mem_inf(&inf_data[total_inf_len], &inf_len, data, IDAT_len);
		total_inf_len += inf_len;
	}
    
    U8 def_data[total_def_len];
    U64 def_len;
    
    mem_def(def_data, &def_len, inf_data, total_inf_len, Z_DEFAULT_COMPRESSION);
    
	FILE *fp;
	fp = fopen("all.png", "wb+");

	unsigned char header[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
	fwrite(header, 8*sizeof(unsigned char), 1, fp);

	unsigned char IHDR_len[4] = {0x00, 0x00, 0x00, 0x0d};
	fwrite(IHDR_len, 4*sizeof(unsigned char), 1, fp);

	unsigned char IHDR_type[4] = {0x49, 0x48, 0x44, 0x52};
	fwrite(IHDR_type, 4*sizeof(unsigned char), 1, fp);

    width = htonl(width);
	fwrite(&width, sizeof(width), 1, fp);
    total_height = htonl(total_height);
	fwrite(&total_height, sizeof(total_height), 1, fp);

	unsigned char IHDR_else[5] = {0x08, 0x06, 0x00, 0x00, 0x00};
	fwrite(IHDR_else, 5*sizeof(unsigned char), 1, fp);

	unsigned char *IHDR_type_data = malloc(17*sizeof(unsigned char));
	fseek(fp, IHDR_TYPE_POS, SEEK_SET);
	fread(IHDR_type_data, sizeof(unsigned char), 17, fp);
	unsigned int IHDR_crc;
	IHDR_crc = htonl(crc(IHDR_type_data, 17));
	fwrite(&IHDR_crc, sizeof(unsigned int), 1, fp);

    unsigned int total_IDAT_len;
    total_def_len = htonl(def_len);
    fwrite(&total_def_len, sizeof(total_def_len), 1, fp);

    fwrite(&type, sizeof(type), 1, fp);
    fwrite(def_data, def_len, 1, fp);
    
    unsigned char IDAT_data_type[4 + def_len];
    U32 IDAT_crc;
    fseek(fp, IDAT_TYPE_POS, SEEK_SET);
    fread(IDAT_data_type, 1, 4 + def_len, fp);
    IDAT_crc = htonl(crc(IDAT_data_type, 4 + def_len));
    fwrite(&IDAT_crc, sizeof(unsigned int), 1, fp);
    
    U8 IEND[12] = {0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82};
    fwrite(IEND, sizeof(U8)*12, 1, fp);
    
    fclose(fp);
    
    free(IHDR_type_data);
    return 0;
}

int fetch_done(int seq_arr[]){
    for (int i = 0; i < 50; i++) {
        if (seq_arr[i] == 1) {
            return 0;
        }
    }
    return 1;
}

void* cURL_fetch_PDF(void* para)
{
    char* url = (char*) para;
    
    while (fetch_done(pic_seq) == 0) {
        CURL *curl_handle;
        CURLcode res;
        RECV_BUF recv_buf;
        char fname[256];
        pid_t pid = getpid();

        recv_buf_init(&recv_buf, BUF_SIZE);
        
        
        curl_global_init(CURL_GLOBAL_DEFAULT);

        /*init a curl session*/
        curl_handle = curl_easy_init();
        if(curl_handle == NULL)
        {
            fprintf(stderr, "curl_easy_init: return NULL\n");
            return 1;
        }

        /*specify URL to get*/
        curl_easy_setopt(curl_handle, CURLOPT_URL, url);

        /*register write call back function to process received data*/
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl3);
        /*user defined data structure passed to the call back function*/
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&recv_buf);

        /*register header call back function to process received header data*/
        curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl);
        /*user defined data structure passed to the call back fundtion*/
        curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)&recv_buf);

        /*some servers requires a user-agent filed*/
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        /*get data*/
        res = curl_easy_perform(curl_handle);
        if(res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
//        else
//        {
//            printf("%lu bytes received in memory %p, seq = %d. \n", recv_buf.size, recv_buf.buf, recv_buf.seq);
//        }
        if (pic_seq[recv_buf.seq] == 1) {
            memcpy(buf[recv_buf.seq], recv_buf.buf, recv_buf.size);
            pic_seq[recv_buf.seq]--;
        }
        curl_easy_cleanup(curl_handle);
        curl_global_cleanup();
        recv_buf_cleanup(&recv_buf);
    }
    return 0;
}
