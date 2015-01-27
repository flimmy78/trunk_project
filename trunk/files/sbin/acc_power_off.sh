#!/bin/bash

. /sbin/autelan_functions.sh
file_path=/root/onoff

get_onoff_log() {
	local ontime_file=/root/onoff/ap-on
	local ontime=$( cat ${ontime_file} |sed -n '$p' )       
	local offtime=$(get_time)

	local line=$( grep -n "" ${ontime_file} |wc -l )
	local del_line=$(awk 'BEGIN{printf("%d",'$line'-'2')}')

	if [[ ${line} -gt 2 ]];then
		sed -e "1,${del_line}"d ${ontime_file} -i 2>/dev/null
	fi

	local json=$(json_create_bykvs \
					"ontime" "${ontime}" \
					"offtime" "${offtime}" \
					"offreason" "ACC-OFF")
	echo "${json}" > ${file_path}/ap-on-off-${offtime}
}

main() {
	touch /root/onoff/acc_off.txt
	get_onoff_log
}

main "$@"
