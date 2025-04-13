#!/usr/bin/python

from mininet.net import Mininet
from mininet.node import OVSController
from mininet.cli import CLI
from mininet.log import setLogLevel, info
from mininet.link import TCLink
from time import sleep
import os

def cleanup():
    print("Cleaning up previous Mininet instances...")
    os.system('sudo mn -c')
    os.system('sudo pkill -f controller')
    os.system('sudo service openvswitch-switch restart')
    
    # Create directory for packet captures
    if not os.path.exists('./captures'):
        os.makedirs('./captures')

def createNetwork():
    # Clean up first
    cleanup()
    
    # Create a network with OVSController and enable OpenFlow13
    net = Mininet(controller=OVSController, link=TCLink)
    
    # Add controller
    print("Adding controller...")
    c0 = net.addController('c0')
    
    # Add switches
    print("Adding switches...")
    s1 = net.addSwitch('s1', protocols='OpenFlow13')
    s2 = net.addSwitch('s2', protocols='OpenFlow13')
    s3 = net.addSwitch('s3', protocols='OpenFlow13')
    s4 = net.addSwitch('s4', protocols='OpenFlow13')
    
    # Add hosts with specified IP addresses
    print("Adding hosts...")
    h1 = net.addHost('h1', ip='10.0.0.2/24')
    h2 = net.addHost('h2', ip='10.0.0.3/24')
    h3 = net.addHost('h3', ip='10.0.0.4/24')
    h4 = net.addHost('h4', ip='10.0.0.5/24')
    h5 = net.addHost('h5', ip='10.0.0.6/24')
    h6 = net.addHost('h6', ip='10.0.0.7/24')
    h7 = net.addHost('h7', ip='10.0.0.8/24')
    h8 = net.addHost('h8', ip='10.0.0.9/24')
    
    # Add links between switches with 7ms latency
    print("Adding switch-to-switch links...")
    net.addLink(s1, s2, cls=TCLink, delay='7ms')
    net.addLink(s2, s3, cls=TCLink, delay='7ms')
    net.addLink(s3, s4, cls=TCLink, delay='7ms')
    net.addLink(s4, s1, cls=TCLink, delay='7ms')
    net.addLink(s1, s3, cls=TCLink, delay='7ms')
    
    # Add links between hosts and switches with 5ms latency
    print("Adding host-to-switch links...")
    net.addLink(h1, s1, cls=TCLink, delay='5ms')
    net.addLink(h2, s1, cls=TCLink, delay='5ms')
    net.addLink(h3, s2, cls=TCLink, delay='5ms')
    net.addLink(h4, s2, cls=TCLink, delay='5ms')
    net.addLink(h5, s3, cls=TCLink, delay='5ms')
    net.addLink(h6, s3, cls=TCLink, delay='5ms')
    net.addLink(h7, s4, cls=TCLink, delay='5ms')
    net.addLink(h8, s4, cls=TCLink, delay='5ms')
    
    # Start the network
    print("Starting network...")
    net.build()
    c0.start()
    s1.start([c0])
    s2.start([c0])
    s3.start([c0])
    s4.start([c0])
    
    print("Network created successfully")
    
    # Configure switches for forwarding
    for s in [s1, s2, s3, s4]:
        s.cmd('ovs-ofctl add-flow {} "actions=normal"'.format(s.name))
    
    # Set MAC addresses explicitly to avoid ARP issues
    info("*** Setting static MAC addresses\n")
    h1.setMAC('00:00:00:00:00:01')
    h2.setMAC('00:00:00:00:00:02')
    h3.setMAC('00:00:00:00:00:03')
    h4.setMAC('00:00:00:00:00:04')
    h5.setMAC('00:00:00:00:00:05')
    h6.setMAC('00:00:00:00:00:06')
    h7.setMAC('00:00:00:00:00:07')
    h8.setMAC('00:00:00:00:00:08')
    
    # Set static ARP entries to avoid ARP discovery issues
    info("*** Setting static ARP entries\n")
    for h in net.hosts:
        for target in net.hosts:
            if h != target:
                h.setARP(target.IP(), target.MAC())
    
    # Wait for network to initialize
    print("Waiting for network initialization...")
    sleep(10)
    
    # PART A: Test ping commands (before fix)
    print("\n--- PART A: Testing Ping Commands (Before STP) ---")
    
    # Test 1: Ping h1 from h3
    print("\nTest 1: Ping h1 from h3")
    print(h3.cmd('ping -c 3 -W 2 10.0.0.2'))
    
    # Test 2: Ping h7 from h5
    print("\nTest 2: Ping h7 from h5")
    print(h5.cmd('ping -c 3 -W 2 10.0.0.8'))
    
    # Test 3: Ping h2 from h8
    print("\nTest 3: Ping h2 from h8")
    print(h8.cmd('ping -c 3 -W 2 10.0.0.3'))
    
    # PART B: Fix the problem by enabling STP
    print("\n--- PART B: Enabling STP to fix network loops ---")
    for s in [s1, s2, s3, s4]:
        # Enable STP on each switch
        s.cmd('ovs-vsctl set bridge {} stp_enable=true'.format(s.name))
        # Set priority to make s1 the root bridge
        if s.name == 's1':
            s.cmd('ovs-vsctl set bridge {} other_config:stp-priority=0x1000'.format(s.name))
    
    # Wait longer for STP to converge
    print("Waiting for STP to converge (60 seconds)...")
    sleep(60)
    
    # Show STP status on each switch
    print("\n--- STP Status on Each Switch ---")
    for s in [s1, s2, s3, s4]:
        print(f"\nSwitch {s.name} STP status:")
        print(s.cmd('ovs-vsctl list bridge {} | grep stp'.format(s.name)))
        print(s.cmd('ovs-vsctl get bridge {} stp_enable'.format(s.name)))
        print(s.cmd('ovs-appctl fdb/show {}'.format(s.name)))
        print(s.cmd('ovs-ofctl show {}'.format(s.name)))
    
    # Test pings after STP (with increased timeout)
    print("\n--- Testing Ping Commands (After STP) ---")
    
    # Function to run ping test 3 times with 30s interval and increased timeout
    def run_ping_test(source, dest, dest_ip):
        print(f"\nRunning ping test: {source.name} to {dest.name}")
        # Check connectivity first
        print(f"Checking connectivity from {source.name} to {dest.name}:")
        print(source.cmd(f'traceroute -n -w 2 -m 8 {dest_ip}'))
        
        for i in range(3):
            print(f"Test {i+1}/3:")
            # Increase timeout to 5 seconds and size to make sure it works
            result = source.cmd(f'ping -c 3 -W 5 -s 64 {dest_ip}')
            print(result)
            
            if i < 2:  # If not the last iteration
                print(f"Waiting 30 seconds before next test...")
                sleep(30)
    
    # Run all three ping tests
    run_ping_test(h3, h1, '10.0.0.2')
    run_ping_test(h5, h7, '10.0.0.8')
    run_ping_test(h8, h2, '10.0.0.3')
    
    # Display extra network information
    print("\n--- Interface Information ---")
    for h in net.hosts:
        print(f"\nHost {h.name} interfaces:")
        print(h.cmd('ifconfig'))
    
    for s in [s1, s2, s3, s4]:
        print(f"\nSwitch {s.name} interfaces:")
        print(s.cmd('ifconfig'))
    
    print("\n\nThe network is now fixed using STP")
    print("For example:")
    print("  h3 ping -c 3 10.0.0.2")
    print("  h5 ping -c 3 10.0.0.8")
    print("  h8 ping -c 3 10.0.0.3")
    print("\nType 'exit' to quit Mininet")
    
    # Open CLI for additional manual testing
    CLI(net)
    
    # Stop the network
    net.stop()

if __name__ == '__main__':
    setLogLevel('info')
    createNetwork()
