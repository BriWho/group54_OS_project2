#include <stdio.h>
#include <string.h>
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

size_t get_filesize(const char* filename);//get the size of the input file


int main (int argc, char* argv[])
{
	char buf[BUF_SIZE];
	char *file_addr;
	char *dev_addr;
	int i, dev_fd, file_fd;// the fd for the device and the fd for the input file
	size_t ret, file_size, tmp;
	char file_name[50], method[20];
	char c[3];
	char *kernel_address = NULL, *file_address = NULL;
	struct timeval start;
	struct timeval end;
	double trans_time; //calulate the time between the device is opened and it is closed
	double n_time = 0;
	int n_file;
	char dir[50];
	size_t nfile_size = 0;
	
	strcpy(c,argv[1]);
	n_file = atoi(c);
	strcpy(dir, argv[2]);
	strcpy(method, argv[3]);

	gettimeofday(&start ,NULL);
	
    	for(int i = 0;i < n_file;i++){
		if( (dev_fd = open("/dev/master_device", O_RDWR)) < 0)
		{
			perror("failed to open /dev/master_device\n");
			return 1;
 		}		
		char nfile_name[50];
		sprintf(nfile_name,"%s/target_file_%d", dir, i+1);
		if( (file_fd = open(nfile_name, O_RDWR)) < 0 )
		{
			perror("failed to open input file\n");
			return 1;
		}

		if( (file_size = get_filesize(nfile_name)) < 0)
		{
			perror("failed to get filesize\n");
			return 1;
		}
		
		if(ioctl(dev_fd, 0x12345677) == -1) //0x12345677 : create socket and accept the connection from the slave
		{
			perror("ioclt server create socket error\n");
			return 1;
		}
		//printf("%d\n",file_size);
		size_t offset = 0;
		switch(method[0]) 
		{
			case 'f': //fcntl : read()/write()
				do
				{
					ret = read(file_fd, buf, sizeof(buf)); // read from the input file
					//for(i = 0 ; i < 1 * ret ; i++)
						//printf("%c" , buf[i]);
					write(dev_fd, buf, ret);//write to the the device
				}while(ret > 0);
				break;
        		case 'm':
				do{
					file_addr = mmap(NULL, PAGE_SIZE, PROT_READ, MAP_SHARED, file_fd, offset);					   
					dev_addr = mmap(NULL, PAGE_SIZE, PROT_WRITE, MAP_SHARED, dev_fd, offset);
					size_t t = 0;
					do {	
						size_t length = BUF_SIZE;
						if(BUF_SIZE + offset > file_size)
							length = file_size%BUF_SIZE;
						memcpy(dev_addr, file_addr + t, length);
						while((ioctl(dev_fd, 0x12345678, length)) < 0 && errno == EAGAIN);
						offset += length;
						t += length;
					   }while(offset % PAGE_SIZE != 0 && offset < file_size);

				ioctl(dev_fd , 0 , file_addr);
				munmap(file_addr, PAGE_SIZE);
			} while (offset < file_size);

			break;
		}

		while(ioctl(dev_fd, 0x12345679)  < 0 && errno == EAGAIN); // end sending data, close the connection
		
		gettimeofday(&end, NULL);
		trans_time = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)*0.0001;
		n_time += trans_time;
		nfile_size += file_size;
		close(file_fd);
		close(dev_fd);
		}	
	printf("Transmission time: %lf ms, File size: %ld bytes\n", n_time, nfile_size / 8);
	return 0;
}

size_t get_filesize(const char* filename)
{
	struct stat st;
	stat(filename, &st);
	return st.st_size;
}
