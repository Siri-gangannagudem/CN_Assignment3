#include <stdio.h>
#include "routing.h" // Include header file

#define INFINITY 999   // Represents infinite cost

// Direct link costs from node 2 to other nodes
int direct_link_costs_2[4] = {3, 1, 0, 2};  // Cost to self is 0

// Declare the distance table instance without redefining the structure
struct distance_table dt2;

/**
 * Sends current minimum cost to all directly connected neighbors
 */
void send_to_neighbors2() {
    struct rtpkt pkt;
    pkt.sourceid = 2;

    // Compute mincost[] vector from the distance table
    for (int i = 0; i < 4; i++) {
        int min = INFINITY;
        for (int j = 0; j < 4; j++) {
            if (dt2.costs[i][j] < min)
                min = dt2.costs[i][j];
        }
        pkt.mincost[i] = min;
    }

    // Send the packet to directly connected neighbors only (nodes 0, 1, and 3)
    for (int i = 0; i < 4; i++) {
        if (i != 2 && direct_link_costs_2[i] < INFINITY) {  // Skip self (node 2)
            pkt.destid = i;
            tolayer2(pkt);
        }
    }
}

/**
 * Initializes node 2's distance table and sends initial costs to neighbors
 */
void rtinit2() {
    // Set all costs to INFINITY initially
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            dt2.costs[i][j] = INFINITY;

    // Set cost to each node via itself (i.e., direct link cost)
    for (int i = 0; i < 4; i++)
        dt2.costs[i][i] = direct_link_costs_2[i];

    // Send initial costs to neighbors
    send_to_neighbors2();

    // Print initial distance table
    printdt2(&dt2);
}

/**
 * Called when node 2 receives a routing packet from a neighbor
 */
void rtupdate2(struct rtpkt *rcvdpkt) {
    int src = rcvdpkt->sourceid;
    int updated = 0;

    // Ignore if the received packet is not from a valid neighbor
    if (direct_link_costs_2[src] == INFINITY) return;

    // Update distance table using Bellman-Ford equation
    for (int dest = 0; dest < 4; dest++) {
        int new_cost = direct_link_costs_2[src] + rcvdpkt->mincost[dest];
        if (new_cost < dt2.costs[dest][src]) {
            dt2.costs[dest][src] = new_cost;
            updated = 1;
        }
    }

    // If any update occurred, propagate new costs to neighbors
    if (updated)
        send_to_neighbors2();

    // Print updated distance table
    printdt2(&dt2);
}

/**
 * Prints node 2's distance table.
 * Displays the current costs for all destinations via different neighbors. 
 */
void printdt2(struct distance_table *dtptr) {
    printf("                via     \n");
    printf("   D2 |    0     1    3 \n");
    printf("  ----|-----------------\n");
    printf("     0|  %3d   %3d   %3d\n", dtptr->costs[0][0], dtptr->costs[0][1], dtptr->costs[0][3]);
    printf("dest 1|  %3d   %3d   %3d\n", dtptr->costs[1][0], dtptr->costs[1][1], dtptr->costs[1][3]);
    printf("     3|  %3d   %3d   %3d\n", dtptr->costs[3][0], dtptr->costs[3][1], dtptr->costs[3][3]);
}
