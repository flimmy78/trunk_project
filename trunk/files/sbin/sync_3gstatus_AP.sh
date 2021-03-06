#!/bin/bash

status_i=0

del_core() {
        local core_file=/tmp/wifidog.*.core
        ls -la ${core_file} >/dev/null 2>&1;local core_result=$?

        if [[ ${core_result} -eq 0 ]];then
            rm -rf ${core_file}
        fi
}

status_3g() {
        local inter_evdo=$( /sbin/ifconfig 3g-evdo |/bin/grep addr  2>/dev/null )
        local evdo_ip=$(/sbin/ifconfig 3g-evdo |/bin/grep addr |awk '{print $2}' |awk -F ':' '{print $2}')
        local STATUS_3g
        local FILE=/tmp/.ppp/status
        local file_evdo_ip=/tmp/.ppp/evdo_ip
        local DOWN=down
        local UP=up

        local PS_WFDG=/tmp/.ps_wifidog
        ps |grep wifidog >${PS_WFDG}
        grep /usr/bin/wifidog ${PS_WFDG} >/dev/null 2>&1;local WFDG_stat=$?
        local WIFIDOG=/etc/init.d/wifidog

        if [[ -z ${inter_evdo} ]];then
                echo ${DOWN} >${FILE}
                ((status_i++))
                #if [[ ${WFDG_stat} -eq 1 ]];then
                #        ${WIFIDOG} start >/dev/null 2>&1
                #fi
                if [[ ${status_i} -ge 9 ]];then
                        killall check_3gdown_result.sh 2>/dev/null
                        check_3gdown_result.sh &
                        sleep 15
                        status_i=0
                fi
        else
                echo ${UP} >${FILE}
                status_i=0
                echo ${evdo_ip} >${file_evdo_ip}
                if [[ ${WFDG_stat} -eq 0 ]];then
                        ${WIFIDOG} stop >/dev/null 2>&1
                fi
        fi
        rm -rf ${PS_WFDG} 2>/dev/null
}

main() {
        while :
        do
                del_core 2>/dev/null
                status_3g 2>/dev/null
                sleep 10
        done
}

main "$@"
