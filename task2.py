#!/usr/bin/python

from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import OVSBridge
from mininet.link import TCLink
from mininet.cli import CLI
from mininet.log import setLogLevel
import time
import os

class Topology(Topo):
    def build(self, *args, **kwargs):
        """
        Override the build method to call createNetwork.
        This ensures Mininet correctly builds the topology.
        """
        self.createNetwork()
        
    def createNetwork(self):
        """
        Network topology with public (10.0.0.0/24) and private (10.1.1.0/24) segments.
        Uses NAT gateway (h9) connecting private hosts to public network.
        """
        # Create switches with STP enabled for loop prevention
        s1 = self.addSwitch('s1', stp=True)  
        s2 = self.addSwitch('s2', stp=True)  
        s3 = self.addSwitch('s3', stp=True)
        s4 = self.addSwitch('s4', stp=True)
        s5 = self.addSwitch('s5', stp=True)  # Create switch for private network

        # Public network hosts
        h3 = self.addHost('h3', ip='10.0.0.4/24')
        h4 = self.addHost('h4', ip='10.0.0.5/24')
        h5 = self.addHost('h5', ip='10.0.0.6/24')
        h6 = self.addHost('h6', ip='10.0.0.7/24')
        h7 = self.addHost('h7', ip='10.0.0.8/24')
        h8 = self.addHost('h8', ip='10.0.0.9/24')
        
        # NAT gateway and private hosts (IPs configured later)
        gateway = self.addHost('h9', ip=None)
        h1 = self.addHost('h1', ip=None)
        h2 = self.addHost('h2', ip=None)

        # Connect public hosts to edge switches
        self.addLink(h3, s2, cls=TCLink, delay='5ms')
        self.addLink(h4, s2, cls=TCLink, delay='5ms')
        self.addLink(h5, s3, cls=TCLink, delay='5ms')
        self.addLink(h6, s3, cls=TCLink, delay='5ms')
        self.addLink(h7, s4, cls=TCLink, delay='5ms')
        self.addLink(h8, s4, cls=TCLink, delay='5ms')
        
        # Create backbone network with redundancy
        self.addLink(s1, s2, cls=TCLink, delay='7ms')
        self.addLink(s2, s3, cls=TCLink, delay='7ms')
        self.addLink(s3, s4, cls=TCLink, delay='7ms')
        self.addLink(s4, s1, cls=TCLink, delay='7ms')
        self.addLink(s1, s3, cls=TCLink, delay='7ms') 

        # Connect NAT gateway to public network
        self.addLink(gateway, s1, cls=TCLink, delay='5ms')
        
        # Connect NAT gateway to private network switch
        self.addLink(gateway, s5, cls=TCLink, delay='1ms')
        
        # Connect private hosts to private network switch
        self.addLink(h1, s5, cls=TCLink, delay='5ms')
        self.addLink(h2, s5, cls=TCLink, delay='5ms')


def NATConfig(net):
    """
    Configure NAT gateway and private hosts.
    Sets up IP addressing, routing and firewall rules.
    """
    h1 = net.get('h1')
    h2 = net.get('h2')
    h9 = net.get('h9') 

    # Configure private hosts
    print("* Setting up private hosts h1 and h2")
    h1.cmd("ifconfig h1-eth0 10.1.1.2/24 up")
    h1.cmd("ip route add default via 10.1.1.1")
    
    h2.cmd("ifconfig h2-eth0 10.1.1.3/24 up")
    h2.cmd("ip route add default via 10.1.1.1")

    # Configure NAT gateway interfaces
    print("* Setting up NAT gateway h9")
    # Public interface with public IP
    h9.cmd("ifconfig h9-eth0 10.0.0.1/24 up")
    h9.cmd("ip addr add 172.16.10.10/24 dev h9-eth0")  # Main NAT IP
    h9.cmd("ip addr add 172.16.10.11/24 dev h9-eth0")  # Secondary IP for h1
    h9.cmd("ip addr add 172.16.10.12/24 dev h9-eth0")  # Secondary IP for h2
    
    # Private interface connecting to private network switch
    h9.cmd("ifconfig h9-eth1 10.1.1.1/24 up")

    # Enable packet forwarding
    h9.cmd("sysctl -w net.ipv4.ip_forward=1")

    print("* Configuring firewall rules")
    h9.cmd("iptables -F")
    h9.cmd("iptables -t nat -F")
    h9.cmd("iptables -X")
    h9.cmd("iptables -t nat -X")

    # Basic masquerading for all private hosts
    h9.cmd("iptables -t nat -A POSTROUTING -s 10.1.1.0/24 -o h9-eth0 -j MASQUERADE")

    # Port forwarding for specific hosts
    # Forward external ICMP traffic to h1 when targeting 172.16.10.11
    h9.cmd("iptables -t nat -A PREROUTING -i h9-eth0 -p icmp -d 172.16.10.11 -j DNAT --to-destination 10.1.1.2")
    # Forward external ICMP traffic to h2 when targeting 172.16.10.12
    h9.cmd("iptables -t nat -A PREROUTING -i h9-eth0 -p icmp -d 172.16.10.12 -j DNAT --to-destination 10.1.1.3")
    
    # Forward TCP traffic for iperf to h1
    h9.cmd("iptables -t nat -A PREROUTING -i h9-eth0 -p tcp --dport 5201 -d 172.16.10.11 -j DNAT --to-destination 10.1.1.2:5201")
    # Forward TCP traffic for iperf to h2
    h9.cmd("iptables -t nat -A PREROUTING -i h9-eth0 -p tcp --dport 5201 -d 172.16.10.12 -j DNAT --to-destination 10.1.1.3:5201")

    # Allow forwarding
    h9.cmd("iptables -A FORWARD -i h9-eth1 -o h9-eth0 -j ACCEPT")  # Private to public
    h9.cmd("iptables -A FORWARD -i h9-eth0 -o h9-eth1 -j ACCEPT")  # Public to private

    # Configure public hosts routing
    print("* Setting up public hosts routing...")
    for h_name in ['h3', 'h4', 'h5', 'h6', 'h7', 'h8']:
        h = net.get(h_name)
        h.cmd("ip route add default via 10.0.0.1")


def run_tests(net):
    """
    Run ping and iperf tests to verify NAT functionality.
    """
    print("\n* Running communication tests...")
    
    print("\na) Test communication to an external host from an internal host:")
    print("i) Ping to h5 from h1")
    h1 = net.get('h1')
    print(h1.cmd("ping -c 4 10.0.0.6"))  # h5 IP
    
    print("ii) Ping to h3 from h2")
    h2 = net.get('h2')
    print(h2.cmd("ping -c 4 10.0.0.4"))  # h3 IP
    
    print("\nb) Test communication to an internal host from an external host:")
    print("i) Ping to h1 from h8")
    h8 = net.get('h8')
    print(h8.cmd("ping -c 4 172.16.10.11"))  # h1's NAT IP
    
    print("ii) Ping to h2 from h6")
    h6 = net.get('h6')
    print(h6.cmd("ping -c 4 172.16.10.12"))  # h2's NAT IP
    
    # Display NAT connection tracking
    h9 = net.get('h9')
    print("\n* Current NAT connections:")
    print(h9.cmd("conntrack -L | grep icmp"))
    
    print("\nc) Iperf tests: 3 tests of 120s each.")
    
    # First test: h1 server, h6 client
    print("i) Run iperf3 server in h1 and iperf3 client in h6")
    h1.cmd("iperf3 -s &")
    time.sleep(2)  # Give server time to start
    print(h6.cmd("iperf3 -c 172.16.10.11 -t 120"))
    h1.cmd("pkill -f iperf3")
    
    # Second test: h8 server, h2 client
    print("ii) Run iperf3 server in h8 and iperf3 client in h2")
    h8.cmd("iperf3 -s &")
    time.sleep(2)  # Give server time to start
    print(h2.cmd("iperf3 -c 10.0.0.9 -t 120"))
    h8.cmd("pkill -f iperf3")
    


def clean_mininet():
    """Clean up any old Mininet processes"""
    os.system('mn -c >/dev/null 2>&1')
    os.system('killall -9 iperf3 >/dev/null 2>&1')
    os.system('sysctl -w net.ipv4.ip_forward=0 >/dev/null 2>&1')


def run():
    """
    Create and run the network topology.
    """
    # Clean up any existing Mininet state
    clean_mininet()
    
    # Create topology and network
    topo = Topology()
    net = Mininet(topo=topo, link=TCLink, switch=OVSBridge, controller=None)

    # Start network and configure NAT
    print("* Starting network...")
    net.start()
    
    # Configure NAT
    NATConfig(net)

    # Wait for STP convergence
    print("* Waiting for network convergence...")
    time.sleep(15)
    print("* Network ready")

    # Run automated tests
    run_tests(net)

    # Start CLI for manual testing
    CLI(net)
    
    # Display NAT rules
    h9 = net.get('h9')
    print("\n* Current NAT rules:")
    print(h9.cmd("iptables -t nat -L -v -n"))
    print("\n* Current forwarding rules:")
    print(h9.cmd("iptables -L FORWARD -v -n"))
    
    # Clean up
    net.stop()
    clean_mininet()


if __name__ == '__main__':
    setLogLevel('info')
    run()
