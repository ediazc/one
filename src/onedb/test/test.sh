#!/bin/bash

################################################################################
# Initialization
################################################################################

if [ -z $ONE_LOCATION ]; then
    echo "ONE_LOCATION not defined."
    exit -1
fi

ONEDCONF_LOCATION="$ONE_LOCATION/etc/oned.conf"

if [ -f $ONEDCONF_LOCATION ]; then
    echo "$ONEDCONF_LOCATION has to be overwritten, move it to a safe place."
    exit -1
fi

VAR_LOCATION="$ONE_LOCATION/var"

if [ -f $VAR_LOCATION/one.db ]; then
    echo "$VAR_LOCATION/one.db has to be overwritten, move it to a safe place."
    exit -1
fi


cp oned.conf $ONEDCONF_LOCATION

export ONE_XMLRPC=http://localhost:2888/RPC2
export PATH=$ONE_LOCATION/bin:$PATH

PID=$$

mkdir results

################################################################################
# Start OpenNebula and populate a DB
################################################################################

echo "Starting oned, some resources will be created"

oned -f &
sleep 2s;

./create.sh

pkill -P $PID oned
sleep 2s;
pkill -9 -P $PID oned

################################################################################
# Upgrade the 2.2 sample DB, and compare schemas
################################################################################

echo "All resources created, now 2.2 DB will be upgraded."

cp $VAR_LOCATION/one.db results/one.db.3.0
cp 2.2/one.db results/one.db.upgraded

onedb upgrade -v --sqlite results/one.db.upgraded --backup results/one.db.backup

echo "Done. Upgraded DB and the one just created will be compared."

# Dump both DB schemas
sqlite3 results/one.db.upgraded ".schema" > results/one.db.upgraded.schema
sqlite3 results/one.db.3.0 ".schema"      > results/one.db.3.0.schema

# Sort the files contents, to avoid false diff errors
sort results/one.db.upgraded.schema > results/one.db.upgraded.schema
sort results/one.db.3.0.schema > results/one.db.3.0.schema


# Perform a diff
FILE=results/schema.diff
diff results/one.db.upgraded.schema results/one.db.3.0.schema > $FILE


if [[ -s $FILE ]] ; then
    echo "Error: Schemas do not match. Check file $FILE"
    exit -1
fi


################################################################################
# Start oned using the upgraded DB and compare objects XMLs
################################################################################

echo "Schemas match. OpenNebula 3.0 will be started with the upgraded 2.2 DB."

cp results/one.db.upgraded $VAR_LOCATION/one.db

oned -f &
sleep 2s;

for obj in host vnet image vm; do
    for i in 0 1 2 3 4; do
        one$obj show -x $i > results/xml_files/$obj-$i-upgraded.xml
    done
done

pkill -P $PID oned
sleep 2s;
pkill -9 -P $PID oned

echo "XML output collected. A diff will be performed."

mkdir results/diff_files

for i in 0 1 2 3 4; do
    diff <(grep -v -e "<LAST_MON_TIME>" -e "<CLUSTER>" -e "NAME>" results/xml_files/host-$i.xml) <(grep -v -e "<LAST_MON_TIME>" -e "<CLUSTER>" -e "NAME>" results/xml_files/host-$i-upgraded.xml) > results/diff_files/host-$i.diff

    diff <(cat results/xml_files/vnet-$i.xml) <(cat results/xml_files/vnet-$i-upgraded.xml) > results/diff_files/vnet-$i.diff

    diff <(grep -v -e "<REGTIME>" -e "<SOURCE>" results/xml_files/image-$i.xml) <(grep -v -e "<REGTIME>" -e "<SOURCE>" results/xml_files/image-$i-upgraded.xml) > results/diff_files/image-$i.diff

    diff <(grep -v -e "<LAST_POLL>" -e "TIME>" -e "<SOURCE>" -e "<TEMPLATE_ID>" -e "<VM_DIR>" results/xml_files/vm-$i.xml) <(grep -v -e "<LAST_POLL>" -e "TIME>" -e "<SOURCE>" -e "<TEMPLATE_ID>" -e "<VM_DIR>" results/xml_files/vm-$i-upgraded.xml) > results/diff_files/vm-$i.diff
done

CODE=0

for obj in host vnet image vm; do
    for i in 0 1 2 3 4; do
        FILE=results/diff_files/$obj-$i.diff
        if [[ -s $FILE ]] ; then
            echo "Error: diff file $FILE is not empty."
            CODE=-1
        fi
    done
done

if [ $CODE -eq 0 ]; then
    echo "Done, all tests passed."
fi

exit $CODE
