## Overview
A rule using a time match applies only to connections that begin during the time frame. If the connection extends past that time frame, it is allowed to continue. That is, the policy is active for a given time frame, and as long as the session is open, traffic can continue to flow.
This solution enables network traffic to occur for a specific length of time. 
At the end of the specified time frame session will be rechecked by the firewall rules. For this reason the matches -m conntrack --ctorigsrc and --ctorigdst should be used instead the -s and -d respectively. Otherwise, all packets in reply destination will be dropped during the recheck stage.
For example, there is the following ruleset:
```
-p udp  -m time --timestart xx:xx --timestop yy:yy --kerneltz -j ACCEPT
-s 10.10.10.10 -p udp -j ACCEPT
```
If a UDP session "10.10.10.10->20.20.20.20" is accepted between xx:xx and yy:yy, it will be rechecked after yy:yy. All packets in the reply direction will be blocked until a packet is passed in the original direction. Using the ctorig match solves this issue.

The rule
```
iptables -A OUTPUT -m recheck --recheck-established -m conntrack --ctstate ESTABLISHED -j ACCEPT
```
should be used instead the rule
```
iptables -A OUTPUT -m conntrack --ctstate ESTABLISHED -j ACCEPT
```
to guarantee that networks sessions are rechecked after the time frame.


## Installation

1. Patch the kernel source.
```
# cd <path to kernel source>
# patch -p1 < <path to timer_recheck source>/patch/recheck.patch
```
2. Build and install the kernel.
3. Copy iptables extensions
```
# cp extensions/* <patch to iptables source>/extensions
```
4. Build and install iptables.
5. Build ipt_tm.ko
```
# cd src
# build
```
6. Load ipt_tm.ko
```
# insmod ipt_tm.ko
```


## Usage
The TACCEPT and TDROP targets have to be used instead the ACCEPT and DROP targets respectively.

## Examples

Example 1. One icmp session is accepted twice for different time frames.

```
iptables -A OUTPUT -m recheck --recheck-established -m conntrack --ctstate ESTABLISHED -j ACCEPT
iptables -A OUTPUT -p icmp -m time --timestart 22:28:20 --timestop 22:28:30 --kerneltz -j TACCEPT
iptables -A OUTPUT -p icmp -m time --timestart 22:28:35 --timestop 22:28:40 --kerneltz -j TACCEPT
iptables -A OUTPUT -p icmp -j TDROP
```

```
# iptables -nvL
Chain INPUT (policy ACCEPT 41 packets, 2996 bytes)
 pkts bytes target     prot opt in     out     source               destination

Chain FORWARD (policy ACCEPT 0 packets, 0 bytes)
 pkts bytes target     prot opt in     out     source               destination

Chain OUTPUT (policy ACCEPT 0 packets, 0 bytes)
 pkts bytes target     prot opt in     out     source               destination
    0     0 ACCEPT     all  --  *      *       0.0.0.0/0            0.0.0.0/0            recheck-established  ctstate ESTABLISHED
    0     0 TACCEPT    icmp --  *      *       0.0.0.0/0            0.0.0.0/0            TIME from 22:28:20 to 22:28:30
    0     0 TACCEPT    icmp --  *      *       0.0.0.0/0            0.0.0.0/0            TIME from 22:28:35 to 22:28:40
    0     0 TDROP      icmp --  *      *       0.0.0.0/0            0.0.0.0/0            
# date
Tue Jan 12 22:28:16 MSK 2021
# ping 192.168.59.54
PING 192.168.59.54 (192.168.59.54) 56(84) bytes of data.
ping: sendmsg: Operation not permitted
ping: sendmsg: Operation not permitted
ping: sendmsg: Operation not permitted
64 bytes from 192.168.59.54: icmp_req=4 ttl=64 time=0.717 ms
64 bytes from 192.168.59.54: icmp_req=5 ttl=64 time=0.175 ms
64 bytes from 192.168.59.54: icmp_req=6 ttl=64 time=0.578 ms
64 bytes from 192.168.59.54: icmp_req=7 ttl=64 time=0.657 ms
64 bytes from 192.168.59.54: icmp_req=8 ttl=64 time=0.639 ms
64 bytes from 192.168.59.54: icmp_req=9 ttl=64 time=0.619 ms
64 bytes from 192.168.59.54: icmp_req=10 ttl=64 time=0.659 ms
64 bytes from 192.168.59.54: icmp_req=11 ttl=64 time=0.456 ms
64 bytes from 192.168.59.54: icmp_req=12 ttl=64 time=0.203 ms
64 bytes from 192.168.59.54: icmp_req=13 ttl=64 time=0.587 ms
64 bytes from 192.168.59.54: icmp_req=14 ttl=64 time=0.673 ms
ping: sendmsg: Operation not permitted
ping: sendmsg: Operation not permitted
ping: sendmsg: Operation not permitted
ping: sendmsg: Operation not permitted
64 bytes from 192.168.59.54: icmp_req=19 ttl=64 time=0.219 ms
64 bytes from 192.168.59.54: icmp_req=20 ttl=64 time=0.675 ms
64 bytes from 192.168.59.54: icmp_req=21 ttl=64 time=0.634 ms
64 bytes from 192.168.59.54: icmp_req=22 ttl=64 time=0.203 ms
64 bytes from 192.168.59.54: icmp_req=23 ttl=64 time=0.606 ms
64 bytes from 192.168.59.54: icmp_req=24 ttl=64 time=0.601 ms
ping: sendmsg: Operation not permitted
ping: sendmsg: Operation not permitted
ping: sendmsg: Operation not permitted
^C
--- 192.168.59.54 ping statistics ---
27 packets transmitted, 17 received, 37% packet loss, time 26026ms
rtt min/avg/max/mdev = 0.175/0.523/0.717/0.189 ms
# iptables -nvL
Chain INPUT (policy ACCEPT 211 packets, 14180 bytes)
 pkts bytes target     prot opt in     out     source               destination

Chain FORWARD (policy ACCEPT 0 packets, 0 bytes)
 pkts bytes target     prot opt in     out     source               destination

Chain OUTPUT (policy ACCEPT 1 packets, 60 bytes)
 pkts bytes target     prot opt in     out     source               destination
   15  1260 ACCEPT     all  --  *      *       0.0.0.0/0            0.0.0.0/0            recheck-established  ctstate ESTABLISHED
    1    84 TACCEPT    icmp --  *      *       0.0.0.0/0            0.0.0.0/0            TIME from 22:28:20 to 22:28:30
    1    84 TACCEPT    icmp --  *      *       0.0.0.0/0            0.0.0.0/0            TIME from 22:28:35 to 22:28:40
   10   840 TDROP      icmp --  *      *       0.0.0.0/0            0.0.0.0/0            
```


Example 2. Standard iptables.

```
iptables -A OUTPUT -m conntrack --ctstate ESTABLISHED -j ACCEPT
iptables -A OUTPUT -p icmp -m time --timestart 22:32:20 --timestop 22:32:30 --kerneltz -j ACCEPT
iptables -A OUTPUT -p icmp -m time --timestart 22:32:35 --timestop 22:32:40 --kerneltz -j ACCEPT
iptables -A OUTPUT -p icmp -j DROP
```

```
# iptables -nvL
Chain INPUT (policy ACCEPT 38 packets, 2792 bytes)
 pkts bytes target     prot opt in     out     source               destination

Chain FORWARD (policy ACCEPT 0 packets, 0 bytes)
 pkts bytes target     prot opt in     out     source               destination

Chain OUTPUT (policy ACCEPT 0 packets, 0 bytes)
 pkts bytes target     prot opt in     out     source               destination
    0     0 ACCEPT     all  --  *      *       0.0.0.0/0            0.0.0.0/0            ctstate ESTABLISHED
    0     0 ACCEPT     icmp --  *      *       0.0.0.0/0            0.0.0.0/0            TIME from 22:32:20 to 22:32:30
    0     0 ACCEPT     icmp --  *      *       0.0.0.0/0            0.0.0.0/0            TIME from 22:32:35 to 22:32:40
    0     0 DROP       icmp --  *      *       0.0.0.0/0            0.0.0.0/0
# date
Tue Jan 12 22:32:16 MSK 2021
# ping 192.168.59.54
PING 192.168.59.54 (192.168.59.54) 56(84) bytes of data.
ping: sendmsg: Operation not permitted
ping: sendmsg: Operation not permitted
ping: sendmsg: Operation not permitted
64 bytes from 192.168.59.54: icmp_req=4 ttl=64 time=0.660 ms
64 bytes from 192.168.59.54: icmp_req=5 ttl=64 time=0.690 ms
64 bytes from 192.168.59.54: icmp_req=6 ttl=64 time=0.697 ms
64 bytes from 192.168.59.54: icmp_req=7 ttl=64 time=0.642 ms
64 bytes from 192.168.59.54: icmp_req=8 ttl=64 time=0.585 ms
64 bytes from 192.168.59.54: icmp_req=9 ttl=64 time=0.206 ms
64 bytes from 192.168.59.54: icmp_req=10 ttl=64 time=0.227 ms
64 bytes from 192.168.59.54: icmp_req=11 ttl=64 time=0.658 ms
64 bytes from 192.168.59.54: icmp_req=12 ttl=64 time=0.115 ms
64 bytes from 192.168.59.54: icmp_req=13 ttl=64 time=0.206 ms
64 bytes from 192.168.59.54: icmp_req=14 ttl=64 time=0.133 ms
64 bytes from 192.168.59.54: icmp_req=15 ttl=64 time=0.698 ms
64 bytes from 192.168.59.54: icmp_req=16 ttl=64 time=0.595 ms
64 bytes from 192.168.59.54: icmp_req=17 ttl=64 time=0.218 ms
64 bytes from 192.168.59.54: icmp_req=18 ttl=64 time=0.187 ms
64 bytes from 192.168.59.54: icmp_req=19 ttl=64 time=0.204 ms
64 bytes from 192.168.59.54: icmp_req=20 ttl=64 time=0.531 ms
64 bytes from 192.168.59.54: icmp_req=21 ttl=64 time=0.704 ms
64 bytes from 192.168.59.54: icmp_req=22 ttl=64 time=1.38 ms
64 bytes from 192.168.59.54: icmp_req=23 ttl=64 time=0.209 ms
64 bytes from 192.168.59.54: icmp_req=24 ttl=64 time=0.657 ms
64 bytes from 192.168.59.54: icmp_req=25 ttl=64 time=0.188 ms
^C
--- 192.168.59.54 ping statistics ---
25 packets transmitted, 22 received, 12% packet loss, time 24016ms
rtt min/avg/max/mdev = 0.115/0.472/1.388/0.303 ms
# date
Tue Jan 12 22:32:43 MSK 2021
# iptables -nvL
Chain INPUT (policy ACCEPT 223 packets, 15556 bytes)
 pkts bytes target     prot opt in     out     source               destination

Chain FORWARD (policy ACCEPT 0 packets, 0 bytes)
 pkts bytes target     prot opt in     out     source               destination

Chain OUTPUT (policy ACCEPT 1 packets, 60 bytes)
 pkts bytes target     prot opt in     out     source               destination
   22  1880 ACCEPT     all  --  *      *       0.0.0.0/0            0.0.0.0/0            ctstate ESTABLISHED
    1    84 ACCEPT     icmp --  *      *       0.0.0.0/0            0.0.0.0/0            TIME from 22:32:20 to 22:32:30
    0     0 ACCEPT     icmp --  *      *       0.0.0.0/0            0.0.0.0/0            TIME from 22:32:35 to 22:32:40
    3   252 DROP       icmp --  *      *       0.0.0.0/0            0.0.0.0/0
```


