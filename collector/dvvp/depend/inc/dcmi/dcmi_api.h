/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#ifndef DCMI_API_H
#define DCMI_API_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*----------------------------------------------*
 * Error code description                       *
 *----------------------------------------------*/

#define DCMI_OK 0
#define DCMI_ERROR_CODE_BASE (-8000)
#define DCMI_ERR_CODE_INVALID_PARAMETER             (DCMI_ERROR_CODE_BASE - 1)
#define DCMI_ERR_CODE_OPER_NOT_PERMITTED            (DCMI_ERROR_CODE_BASE - 2)
#define DCMI_ERR_CODE_MEM_OPERATE_FAIL              (DCMI_ERROR_CODE_BASE - 3)
#define DCMI_ERR_CODE_SECURE_FUN_FAIL               (DCMI_ERROR_CODE_BASE - 4)
#define DCMI_ERR_CODE_INNER_ERR                     (DCMI_ERROR_CODE_BASE - 5)
#define DCMI_ERR_CODE_TIME_OUT                      (DCMI_ERROR_CODE_BASE - 6)
#define DCMI_ERR_CODE_INVALID_DEVICE_ID             (DCMI_ERROR_CODE_BASE - 7)
#define DCMI_ERR_CODE_DEVICE_NOT_EXIST              (DCMI_ERROR_CODE_BASE - 8)
#define DCMI_ERR_CODE_IOCTL_FAIL                    (DCMI_ERROR_CODE_BASE - 9)
#define DCMI_ERR_CODE_SEND_MSG_FAIL                 (DCMI_ERROR_CODE_BASE - 10)
#define DCMI_ERR_CODE_RECV_MSG_FAIL                 (DCMI_ERROR_CODE_BASE - 11)
#define DCMI_ERR_CODE_NOT_REDAY                     (DCMI_ERROR_CODE_BASE - 12)
#define DCMI_ERR_CODE_NOT_SUPPORT_IN_CONTAINER      (DCMI_ERROR_CODE_BASE - 13)
#define DCMI_ERR_CODE_FILE_OPERATE_FAIL             (DCMI_ERROR_CODE_BASE - 14)
#define DCMI_ERR_CODE_RESET_FAIL                    (DCMI_ERROR_CODE_BASE - 15)
#define DCMI_ERR_CODE_ABORT_OPERATE                 (DCMI_ERROR_CODE_BASE - 16)
#define DCMI_ERR_CODE_IS_UPGRADING                  (DCMI_ERROR_CODE_BASE - 17)
#define DCMI_ERR_CODE_RESOURCE_OCCUPIED             (DCMI_ERROR_CODE_BASE - 20)
#define DCMI_ERR_CODE_PARTITION_NOT_RIGHT           (DCMI_ERROR_CODE_BASE - 22)
#define DCMI_ERR_CODE_CONFIG_INFO_NOT_EXIST         (DCMI_ERROR_CODE_BASE - 23)
#define DCMI_ERR_CODE_NOT_SUPPORT                   (DCMI_ERROR_CODE_BASE - 255)


struct dcmi_network_pkt_stats_info {
    unsigned long long mac_tx_mac_pause_num;
    unsigned long long mac_rx_mac_pause_num;
    unsigned long long mac_tx_pfc_pkt_num;
    unsigned long long mac_tx_pfc_pri0_pkt_num;
    unsigned long long mac_tx_pfc_pri1_pkt_num;
    unsigned long long mac_tx_pfc_pri2_pkt_num;
    unsigned long long mac_tx_pfc_pri3_pkt_num;
    unsigned long long mac_tx_pfc_pri4_pkt_num;
    unsigned long long mac_tx_pfc_pri5_pkt_num;
    unsigned long long mac_tx_pfc_pri6_pkt_num;
    unsigned long long mac_tx_pfc_pri7_pkt_num;
    unsigned long long mac_rx_pfc_pkt_num;
    unsigned long long mac_rx_pfc_pri0_pkt_num;
    unsigned long long mac_rx_pfc_pri1_pkt_num;
    unsigned long long mac_rx_pfc_pri2_pkt_num;
    unsigned long long mac_rx_pfc_pri3_pkt_num;
    unsigned long long mac_rx_pfc_pri4_pkt_num;
    unsigned long long mac_rx_pfc_pri5_pkt_num;
    unsigned long long mac_rx_pfc_pri6_pkt_num;
    unsigned long long mac_rx_pfc_pri7_pkt_num;
    unsigned long long mac_tx_total_pkt_num;
    unsigned long long mac_tx_total_oct_num;
    unsigned long long mac_tx_bad_pkt_num;
    unsigned long long mac_tx_bad_oct_num;
    unsigned long long mac_rx_total_pkt_num;
    unsigned long long mac_rx_total_oct_num;
    unsigned long long mac_rx_bad_pkt_num;
    unsigned long long mac_rx_bad_oct_num;
    unsigned long long mac_rx_fcs_err_pkt_num;
    unsigned long long roce_rx_rc_pkt_num;
    unsigned long long roce_rx_all_pkt_num;
    unsigned long long roce_rx_err_pkt_num;
    unsigned long long roce_tx_rc_pkt_num;
    unsigned long long roce_tx_all_pkt_num;
    unsigned long long roce_tx_err_pkt_num;
    unsigned long long roce_cqe_num;
    unsigned long long roce_rx_cnp_pkt_num;
    unsigned long long roce_tx_cnp_pkt_num;
    unsigned long long roce_err_ack_num;
    unsigned long long roce_err_psn_num;
    unsigned long long roce_verification_err_num;
    unsigned long long roce_err_qp_status_num;
    unsigned long long roce_new_pkt_rty_num;
    unsigned long long roce_ecn_db_num;
    unsigned long long nic_tx_all_pkg_num;
    unsigned long long nic_tx_all_oct_num;
    unsigned long long nic_rx_all_pkg_num;
    unsigned long long nic_rx_all_oct_num;
    long tv_sec;
    long tv_usec;
    unsigned char reserved[64];
};

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // DCMI_API_H
