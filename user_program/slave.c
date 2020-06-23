#include <stdio.h>
#include <string.h>
#include <stdlib.h> //+
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#define PAGE_SIZE 4096
#define BUF_SIZE 512
int main (int argc, char* argv[])
{
	char buf[BUF_SIZE];
	int i, dev_fd, file_fd;// the fd for the device and the fd for the input file
	size_t ret, file_size = 0, data_size = -1;
	char file_name[50];
	char method[20];
	char ip[20];
	struct timeval start;
	struct timeval end;
	char dir[50];
	double trans_time; //calulate the time between the device is opened and it is closed
	char *dev_addr, *file_addr;
	int n_file; 
	size_t nfile_size = 0;
	double n_time = 0;
	size_t length;

	if( (dev_fd = open("/dev/slave_device", O_RDWR)) < 0)//should be O_RDWR for PROT_WRITE when mmap()
	{
		perror("failed to open /dev/slave_device\n");
		return 1;
	}

	gettimeofday(&start ,NULL);
	n_file = atoi(argv[1]);
	strcpy(dir,argv[2]);
	mkdir(dir, 0777);
	strcpy(method , argv[argc - 2]);
	strcpy(ip , argv[argc -1]);

	if(ioctl(dev_fd, 0x12345677, ip) == -1)	//0x12345677 : connect to master in the device
	{
		perror("ioclt create slave socket error\n");
		return 1;
	}

	write(1, "ioctl success\n", 14);
	for(int i = 0;i < n_file;i++){
		char nfile_name[50];
		sprintf(nfile_name,"%s/received_file_%d", dir, i+1);
		//printf("%s\n", nfile_name);
		if( (file_fd = open(nfile_name, O_RDWR | O_CREAT | O_TRUNC)) < 0)
		{
			perror("failed to open input file\n");
			return 1;
		}

		size_t offset = 0;
		switch(method[0])
		{
			case 'f':
				do{
					while((ret = read(dev_fd, buf, sizeof(buf))) < 0 && errno == EAGAIN); 
					write(file_fd, buf, ret); //write to the input file
					file_size += ret;
				}while(ret > 0);
			break;
			case 'm':
				length = 0;
				do{
					if(file_size % PAGE_SIZE == 0){
						ftruncate(file_fd , file_size + PAGE_SIZE);
						file_addr = mmap(NULL, PAGE_SIZE, PROT_WRITE, MAP_SHARED, file_fd, offset);
					}

					while((length = read(dev_fd , buf , sizeof(buf))) < 0 && errno == EAGAIN);
					file_size += length;
					memcpy(file_addr + (offset%PAGE_SIZE), buf , length);
					offset += length;
				
					if(file_size % PAGE_SIZE == 0){
						ioctl(dev_fd , 0 , file_addr);
						munmap(file_addr, PAGE_SIZE);
					}
				} while (length > 0);
				ftruncate(file_fd , file_size);
			break;
		}
		while(ioctl(dev_fd, 0x12345679) < 0 && errno == EAGAIN)
			;// end receiving data, close the connection		
		gettimeofday(&end, NULL);
		trans_time = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)*0.0001;
		n_time += trans_time;
		nfile_size += file_size;
		close(file_fd);
		close(dev_fd);
	}
	
	printf("Transmission time: %lf ms, File size: %d bytes\n", n_time, nfile_size / 8);
	return 0;
}


