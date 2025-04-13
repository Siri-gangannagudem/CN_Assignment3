#include <stdio.h>
#include "routing.h" // Include header file

#define INFINITY 999   // Represents infinite cost

// Direct link costs for node 3 (cost to self is 0)
int direct_link_costs_3[4] = {7, 999, 2, 0};

// Declare the distance table instance without redefining the structure
struct distance_table dt3;

/**
 * Helper function to send the updated minimum cost to all directly connected neighbors.
 * This function builds a routing packet with the current minimum cost and sends it to all
 * directly connected nodes (neighbors).
 */
void send_to_neighbors3() {
    struct rtpkt pkt;
    pkt.sourceid = 3;  // Set the source node as node 3

    // Build the mincost array from the distance table
    for (int i = 0; i < 4; i++) {
        int min = INFINITY;
        // Find the minimum cost to each node via any neighbor
        for (int j = 0; j < 4; j++) {
            if (dt3.costs[i][j] < min)
                min = dt3.costs[i][j];
        }
        pkt.mincost[i] = min;  // Set the minimum cost for each destination
    }

    // Send the packet only to direct neighbors (excluding node 3 itself)
    for (int i = 0; i < 4; i++) {
        if (i != 3 && direct_link_costs_3[i] < INFINITY) {
            pkt.destid = i;  // Set the destination node
            tolayer2(pkt);   // Send the packet to the network layer
        }
    }
}

/**
 * Initialization function for node 3. Initializes the distance table and sends initial costs to neighbors.
 */
void rtinit3() {
    // Initialize the distance table with INFINITY (i.e., no known path)
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            dt3.costs[i][j] = INFINITY;

    // Set the direct link costs to itself (cost to self is 0)
    for (int i = 0; i < 4; i++) {
        dt3.costs[i][i] = direct_link_costs_3[i];
    }

    // Send initial costs to neighbors
    send_to_neighbors3();

    // Print the initial distance table
    printdt3(&dt3);
}

/**
 * Updates the distance table when a packet is received from a neighbor.
 * Uses the Bellman-Ford algorithm to recompute the shortest path costs.
 */
void rtupdate3(struct rtpkt *rcvdpkt) {
    int src = rcvdpkt->sourceid;  // Get the source node ID
    int updated = 0;  // Flag to check if any update happened

    // If the source is not a direct neighbor, ignore the update
    if (direct_link_costs_3[src] >= INFINITY) return;

    // Update the distance table based on the received packet
    for (int dest = 0; dest < 4; dest++) {
        int new_cost = direct_link_costs_3[src] + rcvdpkt->mincost[dest];
        // If a shorter path is found, update the distance table
        if (new_cost < dt3.costs[dest][src]) {
            dt3.costs[dest][src] = new_cost;
            updated = 1;
        }
    }

    // If any update occurred, send the new costs to neighbors
    if (updated) {
        send_to_neighbors3();
    }

    // Print the updated distance table
    printdt3(&dt3);
}

/**
 * Pretty-prints node 3's distance table.
 * Displays the current costs for all destinations via different neighbors.
 */
void printdt3(struct distance_table *dtptr) {
    printf("                via     \n");
    printf("   D3 |    0     1    2 \n");
    printf("  ----|-----------------\n");
    printf("     0|  %3d   %3d   %3d\n", dtptr->costs[0][0], dtptr->costs[0][1], dtptr->costs[0][2]);
    printf("dest 1|  %3d   %3d   %3d\n", dtptr->costs[1][0], dtptr->costs[1][1], dtptr->costs[1][2]);
    printf("     2|  %3d   %3d   %3d\n", dtptr->costs[2][0], dtptr->costs[2][1], dtptr->costs[2][2]);
}
