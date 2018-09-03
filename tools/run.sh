#!/bin/sh

KERNEL=ngk
ISO=ngos.iso
VM=qemu-system-x86_64

DEFAULTS="-cdrom $ISO -vga std -no-reboot -m 5M" 
#NET="-device rtl8139,netdev=net0 -netdev user,id=net0,hostfwd=udp::5555-:5555 -object filter-dump,id=dump0,netdev=net0,file=dump.pcap"
NET=
VIDEO="-display none"
STDIO="-serial stdio"
EXTRA=""
RUN=0

while getopts "dvsimn" opt; do
    case $opt in
        d)
            # Debug
            EXTRA="$EXTRA -S -s"
            ;;
        v)
            # Video on
            VIDEO="" # not disabled
            STDIO="-monitor stdio"
            ;;
        s)
            # so you can add serial back with video
            STDIO="-serial stdio"
            ;;
        i)
            # Show interrupts
            EXTRA="$EXTRA -d int"
            ;;
        m)
            # Monitor
            STDIO="-monitor stdio"
            ;;
        n)
            # dry run
            RUN=1
            ;;
        /?)
            echo "Unknown option: -$OPTARG" >&2
            ;;
    esac
done

echo $VM $DEFAULTS $VIDEO $STDIO $EXTRA $NET
if [ $RUN -eq '0' ]; then
    $VM $DEFAULTS $VIDEO $STDIO $EXTRA $NET
fi
