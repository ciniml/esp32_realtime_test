#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int main(int argc, char* argv[]) 
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(10000);
    addr.sin_addr.s_addr = inet_addr("192.168.2.14");

    uint8_t data[20240];

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    size_t total_bytes_sent = 0;

    while(true) {
        ssize_t bytes_sent = sendto(sock, data, sizeof(data), 0, (const struct sockaddr*)&addr, sizeof(addr));
        if( bytes_sent < 0 ) {
            printf("error\n");
        }
        else {
            total_bytes_sent += bytes_sent;
        }
        {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            size_t elapsed_ns = (ts.tv_sec - start_time.tv_sec)*1000000000ull + (ts.tv_nsec - start_time.tv_nsec);
            if( elapsed_ns >= 1000000000ul ) {
                printf("transfer rate: %0.2lf\n", (total_bytes_sent*1000000000.0)/elapsed_ns);
                start_time = ts;
                total_bytes_sent = 0;
            }
        }
        // wait 100[ms]
        {
            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = 20*1000*1000;
            nanosleep(&ts, &ts);
        }
    }

    return 0;
}