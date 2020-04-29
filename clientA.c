//NAME - AMRATANSHU SHRIVASTAVA
//ID - 2017A7PS0224P

#include "packet.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <stdio.h>  //printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <poll.h>

#define TIMEOUT 2000

int main()
{
    //2 sockets for 2 channels
    int sock1 = socket(AF_INET, SOCK_STREAM, 0);
    int sock2 = socket(AF_INET, SOCK_STREAM, 0);

    if (sock1 < 0 || sock2 < 0)
    {
        printf("Error creating client socket\n");
        exit(0);
    }
    /* Initialize sockaddr_in data structure */
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5001); // port
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* Attempt a connection */
    if (connect(sock1, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\n Error : Connect Failed for Socket 1\n");
        return 1;
    }

    if (connect(sock2, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\n Error : Connect Failed for Socket 2\n");
        return 1;
    }

    //upload the file input.txt to the server using different sockets
    int channelChoice = 0; //initially through this channel

    FILE *fp;
    fp = fopen("input.txt", "r");
    if (fp == NULL)
    {
        printf("Error opening file\n");
        return 1;
    }

    int offset = 0; //number of bytes already sent
    int event0, event1;
    event0 = 1;
    event1 = 1;
    int loop = 1;

    struct pollfd poll0[1];
    struct pollfd poll1[1];
    poll0[0].fd = sock1;
    poll0[0].events = POLLIN;
    poll1[0].fd = sock2;
    poll1[0].events = POLLIN;

    int sendNew0 = 1; //1 if new packet is to be sent on channel 0
    int sendNew1 = 1; //0 iff previous packet is to be sent on channel 1
    PKT pkt0, pkt1;
    PKT ack0, ack1;
    int unacked0 = 0;
    int unacked1 = 0;
    int zeroiter = 0;
    while (1)
    {
        if (loop == 0)
            zeroiter = 1;
        //clearing BUFF here
        char buff[sizeof(pkt0.data)] = {0};

        while (event0 == 0) //keep retransmitting thr channel0
        {
            if (unacked0 == 0)
            {
                unacked0 = 1;
                sendNew0 = 0;
                break;
            }

            write(sock1, &pkt0, sizeof(pkt0));
            printf("Resending on Timeout in channel 0\n");
            printf("SENT PKT: Seq No %d of size %d Bytes from channel 0\n", pkt0.seqno, pkt0.size);
            event0 = poll(poll0, 1, TIMEOUT);
            if (event0 != 0)
            {
                int ackrcv = poll0[0].revents & POLLIN;
                if (ackrcv)
                {

                    read(sock1, &ack0, sizeof(ack0));
                    printf("RCVD ACK: for PKT with Seq. No. %d from channel 0\n", ack0.seqno);
                    unacked0 = 0;
                    sendNew0 = 1;
                }
                else
                {
                    printf("Receive Error on channel 0, POLLIN did not happen\n");
                }
            }
        }

        while (event1 == 0) //keep retransmitting thr channel0
        {
            if (unacked1 == 0)
            {
                unacked1 = 1;
                sendNew1 = 0;
                break;
            }

            write(sock2, &pkt1, sizeof(pkt1));
            printf("Resending on Timeout in channel 1\n");
            printf("SENT PKT: Seq No %d of size %d Bytes from channel 1\n", pkt1.seqno, pkt1.size);

            event1 = poll(poll1, 1, TIMEOUT);
            if (event1 != 0)
            {
                int ackrcv = poll1[0].revents & POLLIN;
                if (ackrcv)
                {

                    read(sock2, &ack1, sizeof(ack1));
                    printf("RCVD ACK: for PKT with Seq. No. %d from channel 1\n", ack1.seqno);
                    sendNew1 = 1;
                }
                else
                {
                    printf("Receive Error on channel 1, POLLIN did not happen\n");
                }
            }
        }

        //if we want to send the old packet only, there is no need to move the file pointers to even make a new packet, skipping that piece of code with a goto

        if (channelChoice == 0 && sendNew0 == 0) //send on channel 0
        {
            write(sock1, &pkt0, sizeof(pkt0));
            printf("SENT PKT: Seq No %d of size %d Bytes from channel 0\n", pkt0.seqno, pkt0.size);
            goto label;
        }
        else if (channelChoice == 1 && sendNew1 == 0) //send on channel 1
        {
            write(sock2, &pkt1, sizeof(pkt1));
            printf("SENT PKT: Seq No %d of size %d Bytes from channel 1\n", pkt1.seqno, pkt1.size);
            goto label;
        }

        //FSEEK to the offset here reqd? NO becaue fread moves the file pointer by itself
        int nread = fread(buff, 1, sizeof(pkt0.data), fp);
        //printf("%d\n",sizeof(pkt1));
        if (nread > 0) //we read succesfully, maybe partially (when the file is about to end, because it is not in the multiples of PACKET_SIZE)
        {
            //making the packet
            offset += nread; //no of bytes read and sent
            PKT newPacket;
            newPacket.channel = channelChoice;
            memcpy(newPacket.data, buff, sizeof(buff));
            newPacket.isLast = (nread < sizeof(pkt0.data));
            newPacket.seqno = offset; //start sending from index = offset again
            newPacket.size = nread;
            newPacket.tag = 0; //DATA

            //send this packet through channelChoice now
            if (channelChoice == 0) //send on channel 0
            {
                pkt0 = newPacket;
                write(sock1, &pkt0, sizeof(pkt0));
                printf("SENT PKT: Seq No %d of size %d Bytes from channel 0\n", pkt0.seqno, pkt0.size);
            }
            else //send on channel 1
            {
                pkt1 = newPacket;
                write(sock2, &pkt1, sizeof(pkt1));
                printf("SENT PKT: Seq No %d of size %d Bytes from channel 1\n", pkt1.seqno, pkt1.size);
            }

            if (nread < sizeof(pkt0.data)) //we have reached the end of file or some error
            {

                if (feof(fp))
                {
                    //enter the Outer while loop one last time - in case this last packet was lost too
                    loop = 0;
                    //printf("\nYou have reached the end of file\n");
                    //break;
                }
                if (ferror(fp))
                {
                    printf("Unexpected Error reading the input text file\n");
                    exit(0);
                }
            }
        }

    label:

        if (channelChoice == 0)
        {
            event0 = poll(poll0, 1, TIMEOUT);
            if (event0 != 0)
            {
                int ackrcv = poll0[0].revents & POLLIN;
                if (ackrcv)
                {
                    read(sock1, &ack0, sizeof(ack0));
                    printf("RCVD ACK: for PKT with Seq. No. %d from channel 0\n", ack0.seqno);
                }
                else
                {
                    printf("Receive Error on channel 0, POLLIN did not happen\n");
                }
            }
        }
        else
        {
            event1 = poll(poll1, 1, TIMEOUT);
            if (event1 != 0)
            {
                int ackrcv = poll1[0].revents & POLLIN;
                if (ackrcv)
                {
                    read(sock2, &ack1, sizeof(ack1));
                    printf("RCVD ACK: for PKT with Seq. No. %d from channel 1\n", ack1.seqno);
                }
                else
                {
                    printf("Receive Error on channel 1, POLLIN did not happen\n");
                }
            }
        }

        if (channelChoice == 0)
            channelChoice = 1;
        else
            channelChoice = 0;

        if (loop == 0 && zeroiter == 1)
            break;
    }

    //close both the sockets here
    close(sock1);
    printf("Socket 1 is closed.\n");
    close(sock2);
    printf("Socket 2 is closed.\n");

    return 0;
}
