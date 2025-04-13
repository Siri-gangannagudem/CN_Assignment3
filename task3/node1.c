#include <stdio.h>
#include "routing.h" // Include header file

#define INFINITY 999   // Represents infinite cost

// Direct link costs from node 1 to other nodes
int direct_link_costs_1[4] = { 1, 0, 1, 999 };  // Cost to self is 0, others as defined

// Declare the distance table instance without redefining the structure
struct distance_table dt1;

/**
 * Sends current minimum cost to all directly connected neighbors
 */
void send_to_neighbors1() {
    struct rtpkt pkt;
    pkt.sourceid = 1;

    // Compute mincost[] vector from the distance table (minimum cost to each destination)
    for (int i = 0; i < 4; i++) {
        int min = INFINITY;
        for (int j = 0; j < 4; j++) {
            if (dt1.costs[i][j] < min)
                min = dt1.costs[i][j];
        }
        pkt.mincost[i] = min;  // Assign minimum cost to packet for each destination
    }

    // Send the packet to directly connected neighbors (nodes 0 and 2)
    for (int i = 0; i < 4; i++) {
        if (i != 1 && direct_link_costs_1[i] < INFINITY) {
            pkt.destid = i;
            tolayer2(pkt);  // Send the packet to the neighbor
        }
    }
}

/**
 * Initializes node 1's distance table and sends initial costs to neighbors
 */
void rtinit1() {
    // Set all costs to INFINITY initially
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            dt1.costs[i][j] = INFINITY;
        }
    }

    // Set cost to each node via itself (i.e., direct link cost)
    for (int i = 0; i < 4; i++) {
        dt1.costs[i][i] = direct_link_costs_1[i];
    }

    // Send initial costs to neighbors
    send_to_neighbors1();

    // Print initial distance table
    printdt1(&dt1);
}

/**
 * Called when node 1 receives a routing packet from a neighbor
 */
void rtupdate1(struct rtpkt *rcvdpkt) {
    int src = rcvdpkt->sourceid;  // Source node ID from received packet
    int updated = 0;  // Flag to check if distance table is updated

    // Update distance table using Bellman-Ford equation
    for (int dest = 0; dest < 4; dest++) {
        int new_cost = direct_link_costs_1[src] + rcvdpkt->mincost[dest];  // New cost calculation
        if (new_cost < dt1.costs[dest][src]) {  // If new cost is better, update table
            dt1.costs[dest][src] = new_cost;
            updated = 1;
        }
    }

    // If any update occurred, propagate new costs to neighbors
    if (updated) {
        send_to_neighbors1();
    }

    // Print updated distance table
    printdt1(&dt1);
}

/**
 * Prints node 1's distance table.
 * Displays the current costs for all destinations via different neighbors.
 */
void printdt1(struct distance_table *dtptr) {
    printf("                via     \n");
    printf("   D1 |    0     2    3 \n");
    printf("  ----|-----------------\n");
    printf("     0|  %3d   %3d   %3d\n", dtptr->costs[0][0], dtptr->costs[0][2], dtptr->costs[0][3]);
    printf("dest 2|  %3d   %3d   %3d\n", dtptr->costs[2][0], dtptr->costs[2][2], dtptr->costs[2][3]);
    printf("     3|  %3d   %3d   %3d\n", dtptr->costs[3][0], dtptr->costs[3][2], dtptr->costs[3][3]);
}

/**
 * Optional: Handles changes in direct link costs at runtime
 */
void linkhandler1(int linkid, int newcost) {
    int oldcost = direct_link_costs_1[linkid];

    // If the cost hasn't changed, return
    if (oldcost == newcost) return;

    // Update the direct link cost and the distance table
    direct_link_costs_1[linkid] = newcost;
    dt1.costs[linkid][linkid] = newcost;

    int updated = 0;  // Flag to check if any update occurs

    // Recompute costs using the new direct link cost
    for (int dest = 0; dest < 4; dest++) {
        int new_link_cost = direct_link_costs_1[linkid] + dt1.costs[dest][linkid];  // Recalculate the new path cost
        if (new_link_cost < dt1.costs[dest][linkid]) {
            dt1.costs[dest][linkid] = new_link_cost;
            updated = 1;
        }

        // Recalculate minimum cost for each destination
        int current_min = INFINITY;
        for (int j = 0; j < 4; j++) {
            if (dt1.costs[dest][j] < current_min) {
                current_min = dt1.costs[dest][j];
            }
        }

        // If the previous best cost was from the old path, update it
        if (current_min != dt1.costs[dest][linkid]) {
            dt1.costs[dest][linkid] = current_min;
            updated = 1;
        }
    }

    // If any update occurred, propagate new costs to neighbors
    if (updated) {
        send_to_neighbors1();
    }

    // Print updated distance table
    printdt1(&dt1);
}
