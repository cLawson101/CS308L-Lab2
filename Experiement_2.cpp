#include <iostream>

#include <algorithm>
#include <cstring>
#include <stdint.h>
#include <malloc.h>

// for mmap:
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

// for core consolidation
#include <sched.h>

// for timing information
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>

#include <chrono>

#define IO_SIZE 4096

using namespace std;

void shuffle(uint64_t *array, size_t n)
{
  if (n > 1) {
    size_t i;
    for (i = 0; i < n - 1; i++) {
      size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
      uint64_t t = array[j];
      array[j] = array[i];
      array[i] = t;
    }
  }
}

void do_file_io(int fd, char *buf,
      uint64_t *offset_array, size_t n, int opt_read){
        int ret = 0;

        for(int i = 0; i < n; i++){
                ret = lseek(fd, offset_array[i], SEEK_SET);
                if(ret == -1) {
                        perror("lseek");
                        exit(-1);
                }

                if(opt_read){
                        ret = read(fd, buf, IO_SIZE);
                }
                else{
                        ret = write(fd, buf, IO_SIZE);
                }
                if(ret == -1){
                        perror("read/write");
                        exit(-1);
                }
        }
}

uint64_t* getOffset(int size, int incr){

        uint64_t numIncr = size / incr;

        uint64_t* offsets = (uint64_t*)malloc(sizeof(uint64_t) * numIncr);

        offsets[0] = 0;

        for(int i = 1; i < numIncr; i++){
                offsets[i] = offsets[i-1] + incr;
        }

        return offsets;
}

int main(){
        // Core consolidation
        cpu_set_t my_set;
        CPU_ZERO(&my_set);
        CPU_SET(1, &my_set);
        sched_setaffinity(0, sizeof(cpu_set_t), &my_set);

        const char* files[1] = {
        "../fuse-tutorial-2018-02-04/example/mountdir/1GB_file"
        };

        int file_size[1] = {
                1 * 1024 * 1024 * 1024
        };

        int modifySize[1] = {
                256
        };

        using std::chrono::high_resolution_clock;
        using std::chrono::duration_cast;
        using std::chrono::duration;
        using std::chrono::milliseconds;

        uint64_t* offsets = getOffset(file_size[0], 256);

        auto t1 = high_resolution_clock::now();

	shuffle(offsets, file_size[0] / 256);

        int fd = open(files[0], O_RDWR);

	printf("Fd is: %d\n", fd);


        for(int i = 0; i < 10; i++){
                const char* cur_file = files[0];
                int size = file_size[0];

                for(int j = 0; j < 1; j++){
                        int modify = modifySize[j];

                        int numIncr = size / modify;

                        char* buf = (char*) aligned_alloc(modify, size);

                        auto t3 = high_resolution_clock::now();

                        do_file_io(fd, buf, offsets, numIncr, 1);

                        auto t4 = high_resolution_clock::now();
                        duration<double, std::milli> ms_double = t4 - t3;
                        printf("Round %2d time: %f\n", i, ms_double.count() / 1000);


                }


        }
        close(fd);
        auto t2 = high_resolution_clock::now();
        duration<double, std::milli> ms_double = t2 - t1;
        printf("\nTotal time(s): %f\n", ms_double.count() / 1000);
}
