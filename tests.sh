#!/usr/bin/env bash

NETNS_MAJOR=net

INTERFACE_TYPE=veth
INTERFACE_MAJOR="$INTERFACE_TYPE"

netns.up() # minor [subnet_cidr]
{
	local minor="$1"
	local ip_cidr="${2:-10.$minor.0.1/24}"

	local netns_name="$NETNS_MAJOR$minor"
	local interface_name="$INTERFACE_MAJOR$minor"
	local interface_bridge_name="$interface_name-br"

	# Create netns
	ip netns add "$netns_name"

	# Add bridged virtual interface
	ip link add "$interace_name" type "$INTERFACE_TYPE" peer name "$interface_bridge_name"

	# Associate virtual interface with netns
	ip link set "$interface_name" netns "$netns_name"

	# Add virtual interface IP address inside netns
	ip netns exec "$netns_name" ip addr add "$ip_cidr" dev "$interface_name"

	# Add bridge interface IP address inside parent netns
	ip addr "$ip_cidr" dev "$interface_bridge_name"

	# Bring virtual interface up inside netns
	ip netns exec "$netns_name" ip link set "$interface_name" up

	# Bring bridge interface up inside parent netns
	ip link set "$interface_bridge_name" up
}

netns.down() # minor
{
	local minor="$1"

	local netns_name="$NETNS_MAJOR$minor"
	local interface_name="$INTERFACE_MAJOR$minor"
	local interface_bridge_name="$interface_name-br"

	# Bring bridge interface down inside parent netns
	ip link set "$interface_bridge_name" down

	# Bring virtual interface down inside netns
	ip netns exec "$netns_name" ip link set "$interface_name" down

	# Remove bridge interface IP address inside parent netns
	ip link delete "$interface_bridge_name"

	# Remove network namespace
	ip netns delete "$netns_name"
}

netns.exec() # minor [cmd...]
{
	local minor="$1"; shift

	local netns_name="$NETNS_MAJOR$minor"

	ip netns exec "$netns_name" "$@"
}

IP_CIDR_ALICE="10.1.0.1/24"
IP_CIDR_BOB="10.2.0.1/24"

# Bring virtual networks up
netns.up alice "$IP_CIDR_ALICE"
netns.up bob "$IP_CIDR_BOB"

# Enable IPv4 forwarding
echo 1 > /proc/sys/net/ipv4/ip_forward

netns.exec alice ip a
netns.exec bob ip a

netns.down alice
netns.down bob

