#!/bin/sh

rm -fr ./p0/ ./p1/ ./p2/ ./p3/ ./p4/ ./p5/ ./p6/ ./p7 ./p8 ./p9/
mkdir -p ./p0 ./p1 ./p2 ./p3 ./p4 ./p5 ./p6 ./p7 ./p8 ./p9

/bin/cp chord_peer chord_client ./p0/
/bin/cp chord_peer chord_client ./p1/
/bin/cp chord_peer chord_client ./p2/
/bin/cp chord_peer chord_client ./p3/
/bin/cp chord_peer chord_client ./p4/
/bin/cp chord_peer chord_client ./p5/
/bin/cp chord_peer chord_client ./p6/
/bin/cp chord_peer chord_client ./p7/
/bin/cp chord_peer chord_client ./p8/
/bin/cp chord_peer chord_client ./p9/

rfc_list=`ls ./sample_RFCs/`
j=0

for i  in $rfc_list; do
	src_file_name="./sample_RFCs/$i"
	dest_file_name="$j$i"
	/bin/cp  $src_file_name ./p0/$dest_file_name
#	/bin/cp  $src_file_name ./p0/$j
	j=`expr $j + 1`
done

