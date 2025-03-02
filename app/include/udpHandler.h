#ifndef _UDP_HANDLER_H_
#define _UDP_HANDLER_H_

void udpHandler_init();
void udpHandler_recieve(char*);
void udpHandler_reply(char*);
void udpHandler_close();

#endif