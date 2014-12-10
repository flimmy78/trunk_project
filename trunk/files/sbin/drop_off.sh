#!/bin/sh

local file_path=/root/onoff

get_onoff_log() {
	local file_on=ap-on
	local file_off=ap-off
	local ontime=$( cat ${file_path}/${file_on} |sed -n '$p' )
	local offtime=$(cat ${file_path}/${file_off})
	local line=$( grep -n "" ${file_path}/${file_on} |wc -l )
	local del_line=$(awk 'BEGIN{printf("%d",'$line'-'2')}')
	
	if [[ -e ${file_path}/acc_off.txt ]];then                              
		rm -rf ${file_path}/acc_off.txt;
		return 0;                             
	else 
		if [[ ${line} -gt 2 ]];then                                     
			sed -e "1,${del_line}"d ${file_path}/${file_on} -i 2>/dev/null
		fi
	
		echo "{\"ontime\":\"${ontime}\",\"offtime\":\"${offtime}\",\"offreason\":\"DROP-OFF\"}" >${file_path}/ap-on-off-${offtime}
	fi
}

main() {
#	sleep 5
	get_onoff_log
}

main "$@"