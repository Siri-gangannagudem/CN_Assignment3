#ifndef ROUTING_H
#define ROUTING_H

#include <stdio.h>
#include <stdlib.h>

/* Event type constant */
#define FROM_LAYER2 2   /* packet from layer 2 has arrived */

/* Distance table structure */
struct distance_table {
  int costs[4][4];
};

/* Packet structure */
struct rtpkt {
  int sourceid;       /* id of sending router sending this pkt */
  int destid;         /* id of router to which pkt being sent */
  int mincost[4];     /* min cost to node 0 ... 3 */
};

/* Event structure */
struct event {
  float evtime;           /* event time */
  int evtype;             /* event type code */
  int eventity;           /* entity where event occurs */
  struct rtpkt *rtpktptr; /* ptr to packet (if any) associated with this event */
  struct event *prev;
  struct event *next;
};

/* Function declarations for distance_vector.c */
void creatertpkt(struct rtpkt *initrtpkt, int srcid, int destid, int mincosts[]);
int insertevent(struct event *p);
int tolayer2(struct rtpkt packet);
float jimsrand(void);
void printevlist(void);
void init(void);

/* Function declarations for node0.c */
void rtinit0(void);
void rtupdate0(struct rtpkt *rcvdpkt);
void printdt0(struct distance_table *dtptr);
void linkhandler0(int linkid, int newcost);

/* Function declarations for node1.c */
void rtinit1(void);
void rtupdate1(struct rtpkt *rcvdpkt);
void printdt1(struct distance_table *dtptr);
void linkhandler1(int linkid, int newcost);

/* Function declarations for node2.c */
void rtinit2(void);
void rtupdate2(struct rtpkt *rcvdpkt);
void printdt2(struct distance_table *dtptr);

/* Function declarations for node3.c */
void rtinit3(void);
void rtupdate3(struct rtpkt *rcvdpkt);
void printdt3(struct distance_table *dtptr);

#endif /* ROUTING_H */