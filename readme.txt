//NAME - AMRATANSHU SHRIVASTAVA
//ID - 2017A7PS0224P

File input.txt is uploaded from the client to the server (server creates a new file  "uploaded.txt")

EXECUTION INSTRUCTION: 

- Open 2 terminals
- On Terminal 1, gcc -o serverexe serverA.c
- On Terminal 2, gcc -o clientexe clientA.c
- On Terminal 1, ./serverexe
- On Terminal 2, ./clientexe

MULTIPLE CHANNELS - 

    On the client side - 2 Sockets are open simultaneously and connected to the same server
    These 2 sockets are our 2 TCP channels

    On the Server side - 2 sockets are simultaneously checked for any activity using select() statement
    Master socket is used for checking any incoming new channel connections
    The 2 channels are then checked for activity within the loop in every iteration


IMPLEMENTING MULTIPLE TIMERS - 

    Timer value can be set custom using the #define TIMEOUT macro in clientA.c
    Timers are implemented in clientA.c using poll(). 

    An Alternating behaviour of sending Packets is set up at the client side. 
    
    If a PKT sent from a Channel does not receive an ACK from server, it will let the other channel transmit as long as there is no previous UNACKED packet in this channel.

CHANGING CUSTOM VARIABLES - 
Packet Drop Rate - #define PDR in serverA.c
Timeout in ms - #define TIMEOUT in clientA.c
Packet size - #define PACKET_SIZE in packet.h



