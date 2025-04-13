#include <stdio.h>
#include "routing.h" // Include header file

#define INFINITY 999   // Represents infinite cost

// Direct link costs from node 0 to other nodes
int direct_link_costs[4] = {0, 1, 3, 7};  // Cost to self is 0

// Declare the distance table instance without redefining the structure
struct distance_table dt0;

/**
 * Sends current minimum cost to all directly connected neighbors
 */
void send_to_neighbors() {
    struct rtpkt pkt;
    pkt.sourceid = 0;

    // Compute mincost[] vector from the distance table
    for (int i = 0; i < 4; i++) {
        int min = INFINITY;
        for (int j = 0; j < 4; j++) {
            if (dt0.costs[i][j] < min)
                min = dt0.costs[i][j];
        }
        pkt.mincost[i] = min;
    }

    // Send the packet to directly connected neighbors only
    for (int i = 1; i < 4; i++) {
        if (direct_link_costs[i] < INFINITY) {
            pkt.destid = i;
            tolayer2(pkt);
        }
    }
}

/**
 * Initializes node 0's distance table and sends initial costs to neighbors
 */
void rtinit0() {
    // Set all costs to INFINITY initially
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            dt0.costs[i][j] = INFINITY;

    // Set cost to each node via itself (i.e., direct link cost)
    for (int i = 0; i < 4; i++)
        dt0.costs[i][i] = direct_link_costs[i];

    // Send initial costs to neighbors
    send_to_neighbors();

    // Print initial distance table
    printdt0(&dt0);
}

/**
 * Called when node 0 receives a routing packet from a neighbor
 */
void rtupdate0(struct rtpkt *rcvdpkt) {
    int src = rcvdpkt->sourceid;
    int updated = 0;

    // Update distance table using Bellman-Ford equation
    for (int i = 0; i < 4; i++) {
        int new_cost = direct_link_costs[src] + rcvdpkt->mincost[i];
        if (new_cost < dt0.costs[i][src]) {
            dt0.costs[i][src] = new_cost;
            updated = 1;
        }
    }

    // If any update occurred, propagate new costs to neighbors
    if (updated)
        send_to_neighbors();

    // Print updated distance table
    printdt0(&dt0);
}

/**
 * Prints node 0's distance table.
 * Displays the current costs for all destinations via different neighbors.
 */
void printdt0(struct distance_table *dtptr) {
    printf("                via     \n");
    printf("   D0 |    1     2    3 \n");
    printf("  ----|-----------------\n");
    printf("     1|  %3d   %3d   %3d\n", dtptr->costs[1][1], dtptr->costs[1][2], dtptr->costs[1][3]);
    printf("dest 2|  %3d   %3d   %3d\n", dtptr->costs[2][1], dtptr->costs[2][2], dtptr->costs[2][3]);
    printf("     3|  %3d   %3d   %3d\n", dtptr->costs[3][1], dtptr->costs[3][2], dtptr->costs[3][3]);
}

/**
 * Optional: Handles changes in direct link costs at runtime
 */
void linkhandler0(int linkid, int newcost) {
    int oldcost = direct_link_costs[linkid];

    // No update if the cost hasn't changed
    if (oldcost == newcost) return;

    direct_link_costs[linkid] = newcost;
    dt0.costs[linkid][linkid] = newcost;

    int updated = 0;

    // Recompute costs using the new direct link cost
    for (int i = 0; i < 4; i++) {
        int new_link_cost = direct_link_costs[linkid] + dt0.costs[i][linkid];
        if (new_link_cost < dt0.costs[i][0]) {
            dt0.costs[i][0] = new_link_cost;
            updated = 1;
        }
    }

    // If any update occurred, notify neighbors
    if (updated)
        send_to_neighbors();

    printdt0(&dt0);
}
