#!/bin/bash

uname -a
echo


###################################################################################################
# This first section sets various kernel command line options to help reduce latency.
# The "isolcpus" option is the most important.
# Other options are helpful depending on the hardware on the PC and won't hurt if that hardware is not there
# Options which are commented out might be helpful, but possibly not for all setups and need further testing
###################################################################################################

do_update_grub=0
GRUB_FILE="/etc/default/grub"


ISOLCPUS=`lscpu -p | tac | awk -F, '{ if(FNR==1) {lastcore=$2; cpu=$1; cpu0=$1} else if ($2==lastcore && cpu0>1) {cpu = $1 "," cpu} } END {if(cpu0>0) {print cpu} }'`

if ! grep -vE '^[[:space:]]*#' $GRUB_FILE | grep -E '^[[:space:]]*GRUB_CMDLINE_LINUX_DEFAULT.*isolcpus' > /dev/null
then
	echo "isolcpus not set - setting now."

	sed -i "/^GRUB_CMDLINE_LINUX_DEFAULT=/ s/\"$/ isolcpus=$ISOLCPUS\"/" $GRUB_FILE
	do_update_grub=1
fi

##### I had mixed results with this setting - needs further testing #####
#if ! grep -vE '^[[:space:]]*#' $GRUB_FILE | grep -E '^[[:space:]]*GRUB_CMDLINE_LINUX_DEFAULT.*rcu_nocbs' > /dev/null
#then
#	echo "rcu_nocbs not set - setting now."
#	sed -i "/^GRUB_CMDLINE_LINUX_DEFAULT=/ s/\"$/ rcu_nocbs=$ISOLCPUS\"/" $GRUB_FILE
#	do_update_grub=1
#fi

##### I had mixed results with this setting - needs further testing #####
#if ! grep -vE '^[[:space:]]*#' $GRUB_FILE | grep -E '^[[:space:]]*GRUB_CMDLINE_LINUX_DEFAULT.*nohz_full' > /dev/null
#then
#	echo "nohz_full not set - setting now."
#	sed -i "/^GRUB_CMDLINE_LINUX_DEFAULT=/ s/\"$/ nohz_full=$ISOLCPUS\"/" $GRUB_FILE
#	do_update_grub=1
#fi

if ! grep -vE '^[[:space:]]*#' $GRUB_FILE | grep -E '^[[:space:]]*GRUB_CMDLINE_LINUX_DEFAULT.*intel_idle.max_cstate' > /dev/null
then
	echo "intel_idle.max_cstate not set - setting now."
	sed -i "/^GRUB_CMDLINE_LINUX_DEFAULT=/ s/\"$/ intel_idle.max_cstate=1\"/" $GRUB_FILE
	do_update_grub=1
fi

if ! grep -vE '^[[:space:]]*#' $GRUB_FILE | grep -E '^[[:space:]]*GRUB_CMDLINE_LINUX_DEFAULT.*i915.enable_rc6' > /dev/null
then
	echo "Disabling intel graphics power save option."
	sed -i "/^GRUB_CMDLINE_LINUX_DEFAULT=/ s/\"$/ i915.enable_rc6=0\"/" $GRUB_FILE
	do_update_grub=1
fi

##### "mitigations=off" turns off mitigations for certain security holes.  Using it did seem to improve latency a little in my testing #####
#if ! grep -vE '^[[:space:]]*#' $GRUB_FILE | grep -E '^[[:space:]]*GRUB_CMDLINE_LINUX_DEFAULT.*mitigations' > /dev/null
#then
#	echo "mitigations=off not set - setting now."
#	sed -i "/^GRUB_CMDLINE_LINUX_DEFAULT=/ s/\"$/ mitigations=off\"/" $GRUB_FILE
#	do_update_grub=1
#fi

#if ! grep -vE '^[[:space:]]*#' $GRUB_FILE | grep -E '^[[:space:]]*GRUB_CMDLINE_LINUX_DEFAULT.*nomodeset' > /dev/null
#then
#	echo "nomodeset not set - setting now."
#	sed -i "/^GRUB_CMDLINE_LINUX_DEFAULT=/ s/\"$/ nomodeset\"/" $GRUB_FILE
#	do_update_grub=1
#fi

#if ! grep -vE '^[[:space:]]*#' $GRUB_FILE | grep -E '^[[:space:]]*GRUB_CMDLINE_LINUX_DEFAULT.*usbcore.autosuspend' > /dev/null
#then
#	echo "usbcore.autosuspend=-1 not set - setting now."
#	sed -i "/^GRUB_CMDLINE_LINUX_DEFAULT=/ s/\"$/ usbcore.autosuspend=-1\"/" $GRUB_FILE
#	do_update_grub=1
#fi

if [ $do_update_grub -eq 1 ]; then
	update-grub
fi



###################################################################################################
#
# This section sets IRQ affinity so the IRQs of the realtime NIC are handled on the isolated CPU.
#
# First we check if irqbalance is running and if not we set up a script to move all IRQs off of 
# the isolated CPUs, and then move the IRQs of the realtime NIC onto the isolated CPU
#
# If irqbalance is running we install our irqbalance policy script and set the configuration file to use it.
# irqbalance will automatically keep IRQs off of the isolated CPUs, but this script bans it from moving the
# realtime NIC IRQs and then moves them to the isolated CPU
# 
###################################################################################################


install -d -m 0755 /etc/linuxcnc
if [[ $(nproc --all) -lt 2 ]]; then    ##################################################################
    echo "There is only 1 cpu, so we can't set IRQ affinities"
elif $(systemctl is-active --quiet "irqbalance.service") && [[ $(nproc --all) -gt 2 ]]; then    ##############################################
    echo "irqbalance is running AND there are more than 2 cpus - setting up policy script"

cat <<'EOF' > /etc/linuxcnc/lcnc_irqbalancepolicy.sh
#!/bin/bash

SYS_DEV_PATH=$1
IRQ_NUM=$2

# LASTCPUMASK is set to the last CPU because we want the NIC IRQs handled on the same CPU as the
# LCNC rt task, and that is the one it runs on.
# We do NOT use the isolcpus setting because that could either not be set, or include other CPUs
# which share a cache with the last CPU.
LASTCPUMASK=$( printf "%X" $((1<<`lscpu -e=CPU | tail -1`)) )
NIC=`awk 'BEGIN{nic=""} {if ($1=="iface") {tmpnic = $2} if ($1=="address") {if ($2 == 10.10.10.1) {nic = tmpnic}}} END{print(nic)}' /etc/network/interfaces`

if [ "$NIC" != "" ]; then
    grep $NIC /proc/interrupts | cut -d ":" -f 1 | while read -r i; do
    if [ $i -eq $IRQ_NUM ]; then
        echo "ban=true"
        echo $LASTCPUMASK > /proc/irq/$IRQ_NUM/smp_affinity
    fi
    done
fi

exit 0
EOF

    chmod 0755 /etc/linuxcnc/lcnc_irqbalancepolicy.sh
    sed -i "s|.*IRQBALANCE_ARGS=.*|IRQBALANCE_ARGS=\"--policyscript=/etc/linuxcnc/lcnc_irqbalancepolicy.sh\"|" /etc/default/irqbalance
    # remove the other script from rc.local if it's there from previous run of this script without irqbalance running
    sed -i '/lcnc_setirqaffinities/ d' /etc/rc.local

else    ##################################################################
    echo "irqbalance is not running OR there are only 2 cpus - setting up IRQ affinity script"

cat <<'EOF' > /etc/linuxcnc/lcnc_setirqaffinities.sh
#!/bin/bash

# source the default grub file to get the kernel command line settings
. /etc/default/grub

NIC=`awk 'BEGIN{nic=""} {if ($1=="iface") {tmpnic = $2} if ($1=="address") {if ($2 == 10.10.10.1) {nic = tmpnic}}} END{print(nic)}' /etc/network/interfaces`

# now move all other irqs to the non-isolated CPUs
for i in $(echo "$GRUB_CMDLINE_LINUX_DEFAULT")
do
    case $i in
        isolcpus=*)
        echo "$i"
        ISOL="${i#*=}"
        ;;
    esac
done

if [[ -z "$ISOL" ]]; then  # ISOL variable is empty
    NOISOLMASK=$( printf $((1<<`lscpu -e=CPU | tail -1`)) )
else
    for i in ${ISOL//,/ }
    do
        NOISOLMASK=$(( NOISOLMASK | (1<<$i) ))
    done
fi
DEFAFFINITY=0x$(</proc/irq/default_smp_affinity)
#echo "DEFAFFINITY =" $DEFAFFINITY
NOISOLMASK=$( printf '%X' $(( ~NOISOLMASK & DEFAFFINITY )) )
#echo "NOISOLMASK =" $NOISOLMASK

# LASTCPUMASK is set to the last CPU because we want the NIC IRQs handled on the same CPU as the
# LCNC rt task, and that is the one it runs on.
# We do NOT use the isolcpus setting because that could either not be set, or include other CPUs
# which share a cache with the last CPU.
LASTCPUMASK=$( printf "%X" $((1<<`lscpu -e=CPU | tail -1`)) )

if [[ "$NIC" != "" ]]; then
    NICIRQS=$( grep $NIC /proc/interrupts | cut -d ":" -f 1 )
fi

for IRQ in $(ls /proc/irq)
do
    if [[ -d "/proc/irq/$IRQ" ]]; then
        if [[ " ${NICIRQS[@]} " =~ " ${IRQ} " ]]; then
            # move NIC irqs to the last CPU (where the linuxcnc realtime process runs)
            echo "Setting smp_affinity of IRQ $IRQ to $LASTCPUMASK"
            echo $LASTCPUMASK > /proc/irq/$IRQ/smp_affinity
        else
            # move all other irqs to the non-isolated CPUs
            echo "Setting smp_affinity of IRQ $IRQ to $NOISOLMASK"
            echo $NOISOLMASK > /proc/irq/$IRQ/smp_affinity
        fi
    fi
done

exit 0
EOF

    if [[ ! -e /etc/rc.local ]]; then
        cat <<'EOF' > /etc/rc.local
#!/bin/sh -e
#
# rc.local
#
# This script is executed at the end of each multiuser runlevel.
# Make sure that the script will "exit 0" on success or any other
# value on error.
#
# In order to enable or disable this script just change the execution
# bits.
#
# By default this script does nothing.

/etc/linuxcnc/lcnc_setirqaffinities.sh
exit 0
EOF
    else
        if ! grep lcnc_setirqaffinities /etc/rc.local > /dev/null; then sed -i "/^exit.*/i /etc/linuxcnc/lcnc_setirqaffinities.sh" /etc/rc.local; fi
    fi
    chmod 0755 /etc/linuxcnc/lcnc_setirqaffinities.sh
    chmod 0755 /etc/rc.local

fi    ##################################################################



###################################################################################################
# Uncomment the two lines below to disable syncing the system clock to internet time servers.
# This synchronization can cause serious latency and realtime errors.
# Maybe this should be uncommented by default???
###################################################################################################

#systemctl stop systemd-timesyncd
#systemctl disable systemd-timesyncd

exit 0

