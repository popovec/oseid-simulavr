#! /usr/bin/env python
#
# $Id: irq.py,v 1.1 2003/11/29 09:43:14 troth Exp $
#
# Use this script to generate new gdb commands for telling the simulator to
# fire off an irq.
#

BASE_IRQ = 80

for i in range(1,35):
    print
    print "define irq_%d" % (i)
    print "  signal SIG%d" % (BASE_IRQ+i)
    print "end"
