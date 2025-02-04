#!/bin/bash -x

torrent_client_binary=$1
echo "Testing program $torrent_client_binary"

percent=5
echo "First ${percent}% of pieces of the downloaded file must be downloaded correctly to pass test"

torrent_file=/home/lidochek/mephi/lidochek/project-part-7/torrent-client-cli/resources/debian.iso.torrent
# torrent_file=/home/lidochek/mephi/lidochek/project-part-7/torrent-client-cli/resources/debian-9.3.0-ppc64el-netinst.torrent


random_dir=`mktemp -d`
trap 'rm -rf -- "$random_dir"' EXIT

downloaded_file=$random_dir/debian-11.6.0-amd64-netinst.iso
# downloaded_file=downloaded_file=$random_dir/debian-9.3.0-ppc64el-netinst.iso

$torrent_client_binary -d $random_dir -p $percent $torrent_file

# python3 checksum.py -p $percent $torrent_file $downloaded_file
# checksum_result=$?

# exit $checksum_result
