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
#include <errno.h>
#include <unistd.h>   //close
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <time.h>

#define PDR 0.10 //Packet Drop Rate

int randomWithProb(double pdr)
{
    double rndDouble = (double)rand() / RAND_MAX;
    return rndDouble > pdr;
}

int main()
{
    srand(time(NULL));

    int opt = 1;
    int master_socket, addrlen, new_socket, client_socket[2],
        max_clients = 2, activity, i, valread, sd;
    int max_sd;
    struct sockaddr_in address;

    fd_set readfds;
    for (i = 0; i < max_clients; i++)
    {
        client_socket[i] = 0;
    }

    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(5001);

    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", 5001);

    if (listen(master_socket, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(address);
    puts("Waiting for connections ...");

    printf("Master Socket No: %d\n", master_socket);

    PKT temp;
    int expectedMaxSeq = sizeof(temp.data);

    FILE *fp;
    fp = fopen("uploaded.txt", "w");

    if (fp == NULL)
    {
        printf("Error in opening uploaded.txt file");
        exit(0);
    }
    printf("Uploading to the file: uploaded.txt");

    while (1)
    {
        //clear the socket set
        //printf("Debug: Clearing socket set\n");
        FD_ZERO(&readfds);

        //add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        //add child sockets to set
        for (i = 0; i < max_clients; i++)
        {
            //socket descriptor
            sd = client_socket[i];

            //if valid socket descriptor then add to read list
            if (sd > 0)
                FD_SET(sd, &readfds);

            //highest file descriptor number, need it for the select function
            if (sd > max_sd)
                max_sd = sd;
        }

        //wait for an activity on one of the sockets , timeout is NULL ,
        //so wait indefinitely
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR))
        {
            printf("SELECT() error");
        }

        if (FD_ISSET(master_socket, &readfds))
        {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            printf("New connection , socket fd is %d , ip is : %s , port : %d \n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            //add new socket to array of sockets
            for (i = 0; i < max_clients; i++)
            {
                //if position is empty
                if (client_socket[i] == 0)
                {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets at index %d\n\n", i);

                    break;
                }
            }
        }
        PKT orderBuff;
        strcpy(orderBuff.data, "");
        PKT readpkt;
        //int lastSeq;
        //else its some IO operation on some other socket

        for (i = 0; i < max_clients; i++)
        {
            sd = client_socket[i];
            //printf("Checking sd: %d\n", sd);
            if (FD_ISSET(sd, &readfds))
            {
                if ((read(sd, &readpkt, sizeof(readpkt))) < 0)
                {
                    printf("Error in Reading Packets\n");
                    exit(0);
                }

                //Now acc to PDR, either drop the packet or don't
                int dropFlag = randomWithProb(PDR);

                if (!dropFlag) //packet drop, no need to add to orderBuff
                {
                    //not sending ack packet back to the client
                    printf("Oops, a packet was lost\n");
                }
                else
                {
                    //send ACK back, add to orderBuff
                    printf("RCVD PKT: Seq. No %d of size %d Bytes from channel %d\n", readpkt.seqno, readpkt.size, i);
                    PKT ack;
                    ack.channel = i;
                    //ack.data contains garbage
                    //ack.isLast contains garbage
                    ack.seqno = readpkt.seqno;
                    //ack.size contains garbage
                    ack.tag = 1; //ACK

                    if (readpkt.seqno <= expectedMaxSeq)
                    {
                        //it is in order
                        //write readpkt to output file
                        //printf("Debug: readpkt.data is \n\n%s\n\n", readpkt.data);
                        if (readpkt.isLast != 1)
                        {
                            int nwrite = fprintf(fp, "%s", readpkt.data);
                            if (nwrite < 0)
                            {
                                printf("Error in writing to uploaded.txt");
                                exit(0);
                            }
                        }
                        if (strcmp(orderBuff.data, "") != 0)
                        {
                            if (orderBuff.isLast != 1)
                            {
                                //write orderBuff to output file
                                //printf("Debug: orderBuff.data is \n\n%s\n\n", orderBuff.data);
                                int nwrite = fprintf(fp, "%s", orderBuff.data);
                                if (nwrite < 0)
                                {
                                    printf("Error in writing to uploaded.txt");
                                    exit(0);
                                }
                            }
                        }
                    }
                    orderBuff = readpkt;

                    int wr = write(sd, &ack, sizeof(ack));
                    printf("SENT ACK: for PKT with Seq. No. %d from channel %d\n", ack.seqno, i);
                    if (wr < 0)
                    {
                        printf("Error in Sending the ACK packet back\n");
                    }

                    if (readpkt.isLast == 1)
                    {
                        //write this last packet to the file too
                        //printf("Debug:  readpkt.data is \n\n%s\n\n", readpkt.data);

                        if (fprintf(fp, "%s", readpkt.data) < 0)
                        {
                            printf("Error in writing to uploaded.txt");
                        }
                        printf("\n\nPacket with Seq No %d was the last packet. Closing the connections now.\n", readpkt.seqno);
                        close(master_socket);
                        printf("Master Socket closed\n");
                        close(client_socket[0]);
                        printf("Channel 0 closed\n");
                        close(client_socket[1]);
                        printf("Channel 1 closed\n");
                        exit(0);
                    }
                }
                expectedMaxSeq += sizeof(readpkt.data);
            }
        }
    }

    return 0;
}