#!/bin/bash

. ${__ROOTFS__}/sbin/autelan_functions.in
. ${__ROOTFS__}/etc/jsock/msg/msg.in
ERROR_NOSUPPORT="Not supported"

get_gps_time() {
    local time=$(cat /tmp/.gps/gps_time)
    [[ ${time} ]] && echo ${time}
}

get_gps_satellite() {
    local satellite=$(cat /tmp/.gps/gps_satellite)
    [[ ${satellite} ]] && echo ${satellite}
}

get_gps_antenna() {
    echo "${ERROR_NOSUPPORT}"
}
get_gps_antenna_bak() {
    local satellite=$(cat /tmp/.gps/gps_satellite)
    if [[ ${satellite} -eq 0 ]]; then
        echo "open"
    else
        echo "short"
    fi
}

#
# $1: device
#
get_wifi_mode() {
    local device="$1"
    local mode
    local mode_tmp=$(iwinfo ${device} info | awk '/Mode:/{print $2}')

    if [[ "${mode_tmp}" = "Master" ]]; then
        mode="AP"
    else
	mode=${mode_tmp}
    fi
    echo ${mode}
}

#
# $1: device
#
get_wifi_signal() {
    local device="$1"
    local signal=$(iwinfo ${device} info | awk  '/Tx-Power/{print $2}')
    echo ${signal}
}

#
# $1: device
#
get_wifi() {
    local device="$1"
    local mode=$(get_wifi_mode ${device})
    local signal=$(get_wifi_signal ${device})
    echo "${mode}/${signal}"
}

#
# $@: json string
#
str_systemlog_ap() {
    local jsonstr="$@"

    jsonstr=$(add_json_string "date" "$(get_gps_time)" "${jsonstr}")
    jsonstr=$(add_json_string "gps_satellite" "$(get_gps_satellite)" "${jsonstr}")
    jsonstr=$(add_json_string "gps_antenna" "$(get_gps_antenna)" "${jsonstr}")
    jsonstr=$(add_json_string "wifi24" "$(get_wifi wlan0)" "${jsonstr}")
    jsonstr=$(add_json_string "wifi58" "$(get_wifi wlan1)" "${jsonstr}")
    jsonstr=$(add_json_string "3G_net" "$(get_3g_net)" "${jsonstr}")
    jsonstr=$(add_json_string "3g_strong" "$(get_hdrcsq)" "${jsonstr}")
    jsonstr=$(add_json_string "sim-iccid" "$(get_iccid)" "${jsonstr}")

    [[ ${jsonstr} ]] && echo ${jsonstr}
}

service() {
    local jsonstr=""

    jsock_ap_send_check || {
        return ${e_bad_board}
    }

    jsonstr={$(str_systemlog_ap ${jsonstr})}

    ${__ROOTFS__}/etc/jsock/jmsg.sh asyn systemlog_ap "${jsonstr}"
    jmsg_logger "ap send msg json:${jsonstr}"
}

main() {
    while :
    do
	service
	sleep 293
    done    
}

main "$@"
