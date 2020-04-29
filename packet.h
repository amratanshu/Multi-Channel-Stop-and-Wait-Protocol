//NAME - AMRATANSHU SHRIVASTAVA
//ID - 2017A7PS0224P
#include <stdbool.h>

#define PACKET_SIZE 100
typedef struct packet
{

    int size;
    int seqno;
    int isLast;
    int tag; //0 for DATA, 1 for ACK
    int channel;
    char data[PACKET_SIZE - (5 * sizeof(int))];

} PKT;
