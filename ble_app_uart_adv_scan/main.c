/**
 * Copyright (c) 2014 - 2020, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 *
 * @defgroup ble_sdk_uart_over_ble_main main.c
 * @{
 * @ingroup  ble_sdk_app_nus_eval
 * @brief    UART over BLE application main file.
 *
 * This file contains the source code for a sample application that uses the Nordic UART service.
 * This application uses the @ref srvlib_conn_params module.
 */


#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "app_timer.h"
#include "ble_nus.h"
#include "app_uart.h"
#include "app_util_platform.h"
#include "bsp_btn_ble.h"
#include "nrf_pwr_mgmt.h"
#include "app_timer.h"

#if defined (UART_PRESENT)
#include "nrf_uart.h"
#endif
#if defined (UARTE_PRESENT)
#include "nrf_uarte.h"
#endif

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define ADV_FILTER_ON_BUTTON              BSP_BUTTON_0
#define ADV_FILTER_OFF_BUTTON             BSP_BUTTON_1

#define ADV_TURN_ON_BUTTON                BSP_BUTTON_2
#define ADV_TURN_OFF_BUTTON               BSP_BUTTON_3


#define ADV_FILTER_ON_LED                BSP_BOARD_LED_0
#define ADV_FILTER_OFF_LED               BSP_BOARD_LED_1
#define ADVERTISING_LED                  BSP_BOARD_LED_2
#define CONNECTED_LED                    BSP_BOARD_LED_3

#define APP_BLE_CONN_CFG_TAG            1                                           /**< A tag identifying the SoftDevice BLE configuration. */

#define DEVICE_NAME                     "Filter_UART"                               /**< Name of device. Will be included in the advertising data. */
#define NUS_SERVICE_UUID_TYPE           BLE_UUID_TYPE_VENDOR_BEGIN                  /**< UUID type for the Nordic UART Service (vendor specific). */

#define APP_BLE_OBSERVER_PRIO           3                                           /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_SOC_OBSERVER_PRIO           1                                           /**< Applications' SoC observer priority. You shouldn't need to modify this value. */

#define APP_ADV_INTERVAL                160                                         /**< The advertising interval (in units of 0.625 ms. This value corresponds to 40 ms). */

#define APP_ADV_DURATION                0                                       /**< The advertising duration (180 seconds) in units of 10 milliseconds. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(20, UNIT_1_25_MS)             /**< Minimum acceptable connection interval (20 ms), Connection interval uses 1.25 ms units. */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(75, UNIT_1_25_MS)             /**< Maximum acceptable connection interval (75 ms), Connection interval uses 1.25 ms units. */
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units. */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                       /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                      /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define UART_TX_BUF_SIZE                256                                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                256                                         /**< UART RX buffer size. */

#define BUTTON_DETECTION_DELAY APP_TIMER_TICKS(50) /**< Delay from a GPIOTE event until a button is reported as pushed (in number of timer ticks). */

APP_TIMER_DEF(m_timer_id);
BLE_NUS_DEF(m_nus, NRF_SDH_BLE_TOTAL_LINK_COUNT);                                   /**< BLE NUS service instance. */
NRF_BLE_GATT_DEF(m_gatt);                                                           /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                                                             /**< Context for the Queued Write module.*/
//BLE_ADVERTISING_DEF(m_advertising);                                                 /**< Advertising module instance. */

static uint16_t m_conn_handle          = BLE_CONN_HANDLE_INVALID;                   /**< Handle of the current connection. */
static uint16_t m_ble_nus_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;              /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
static ble_uuid_t m_adv_uuids[]          =                                          /**< Universally unique service identifier. */
{
        {BLE_UUID_NUS_SERVICE, NUS_SERVICE_UUID_TYPE}
};

static uint8_t m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;                   /**< Advertising handle used to identify an advertising set. */
static bool m_advertising_is_running = false;
static bool m_advertising_filter_is_running = false;
static const float distance_array[] = {
    0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0,
    1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0,
    2.5, 3.0, 4.5, 5.0, 5.5, 6.0, 6.5, 7.0, 7.5, 8.0,
    8.5, 9.0, 9.5, 10.0, 10.5
};
static int current_distance_index = 0;



static uint8_t m_enc_advdata[BLE_GAP_ADV_SET_DATA_SIZE_MAX];                    /**< Buffer for storing an encoded advertising set. */
static uint8_t m_enc_scan_response_data[BLE_GAP_ADV_SET_DATA_SIZE_MAX];
static bool m_logging_is_running = false;
static uint32_t seconds_since_start = 0;
static bool timer_started = false;



/**@brief Struct that contains pointers to the encoded advertising data. */
static ble_gap_adv_data_t m_adv_data =
{
        .adv_data =
        {
                .p_data = m_enc_advdata,
                .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX
        },
        .scan_rsp_data =
        {
                .p_data = m_enc_scan_response_data,
                .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX

        }
};


/* Scanner configuration */
#define APP_SCAN_SCAN_INTERVAL            (0xA0)     /**< Scanning interval. Determines the scan interval in units of 0.625 millisecond. */
#define APP_SCAN_SCAN_WINDOW              (0xA0)      /**< Scanning window. Determines the scanning window in units of 0.625 millisecond. */

#define APP_SCAN_DURATION           BLE_GAP_SCAN_TIMEOUT_UNLIMITED  /**< Duration of the scanning in units of 10 milliseconds. */
#define APP_SCAN_ACTIVE_DISABLED    0                               /**< Only passive scanning will be processed, no scan request send. */
#define APP_SCAN_ACTIVE_ENABLED     1
#define APP_SCAN_EXTENDED_ENABLED   1
#define APP_SCAN_EXTENDED_DISABLED  0


/**@brief Scan data structure.
 */
typedef struct
{
    bool                  initialized;
    ble_gap_scan_params_t scan_params;              /**< GAP scanning parameters. */
    uint8_t scan_buffer_data[BLE_GAP_SCAN_BUFFER_EXTENDED_MIN];  /**< Buffer where advertising reports will be stored by the SoftDevice. */
    ble_data_t scan_buffer;                         /**< Structure-stored pointer to the buffer where advertising reports will be stored by the SoftDevice. */
} dm_ble_scan_t;

static dm_ble_scan_t     m_scann_ctx;                                        /**< Scanning context data. */


static char const m_target_periph_name[] = DEVICE_NAME;      /**< If you want to connect to a peripheral using a given advertising name, type its name here. */


static void advertising_init(void);
static void advertising_start(void);
static void advertising_stop(void);
static void advertising_with_filter_init(void);
static void advertising_with_filter_start(void);

static ble_gap_addr_t const * m_whitelist_addr_ptrs[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
static ble_gap_addr_t m_whitelist_addrs[BLE_GAP_WHITELIST_ADDR_MAX_COUNT] = {0};
static uint8_t m_addr_cnt = 0;

/** @brief Function compares two device addresses.
 *
 * @param[in] p_addr_1   Address to compare
 * @param[in] p_addr_2   Address to compare
 *
 * @return True if adresses are the same, false otherwise.
 */

static bool addr_cmp(const ble_gap_addr_t * p_addr_1, const ble_gap_addr_t * p_addr_2)
{

        if( p_addr_1->addr[0] == p_addr_2->addr[0] &&
            p_addr_1->addr[1] == p_addr_2->addr[1] &&
            p_addr_1->addr[2] == p_addr_2->addr[2] &&
            p_addr_1->addr[3] == p_addr_2->addr[3] &&
            p_addr_1->addr[4] == p_addr_2->addr[4] &&
            p_addr_1->addr[5] == p_addr_2->addr[5]) {
                return true;
        }
        else {
                return false;
        }
}

/****************************************************************/
static void Get_Connect_MAC_Address(ble_gap_addr_t *gap_address)
{
        NRF_LOG_INFO("Peer Address is %02X:%02X:%02X:%02X:%02X:%02X",
                     gap_address->addr[5],
                     gap_address->addr[4],
                     gap_address->addr[3],
                     gap_address->addr[2],
                     gap_address->addr[1],
                     gap_address->addr[0]);
}

static uint8_t nrf_ble_whitelist_cnt(void)
{
        return m_addr_cnt;
}

static void time_update_handler(void *p_context) {
    seconds_since_start++;
}

void get_timestamp(char *buffer, size_t size) {
    uint32_t total_seconds = seconds_since_start; // Use the global counter
    uint32_t hours = (total_seconds / 3600) % 24;
    uint32_t minutes = (total_seconds / 60) % 60;
    uint32_t seconds = total_seconds % 60;
    sprintf(buffer, "%02lu:%02lu:%02lu", hours, minutes, seconds);
}

static uint32_t nrf_adv_add_whitelist(ble_gap_addr_t *addr, uint8_t * whitelist_count)
{
        uint32_t err_code = NRF_SUCCESS;

        if (m_addr_cnt >= BLE_GAP_WHITELIST_ADDR_MAX_COUNT)
                return NRF_ERROR_DATA_SIZE;

//       for (uint32_t i = 0; i < BLE_GAP_WHITELIST_ADDR_MAX_COUNT; i++)
//        {
//                if (memcmp(&m_whitelist_addrs[i], addr, sizeof(ble_gap_addr_t))==0)
//                {
//                        //NRF_LOG_INFO("[Whitelist]: Duplicate Whitelist add!");
//                        return NRF_ERROR_INVALID_PARAM;
//                }
//        }

        memcpy(&m_whitelist_addrs[m_addr_cnt], addr, sizeof(ble_gap_addr_t));

        m_addr_cnt++;

        //return the number of whitelist store
        *whitelist_count = m_addr_cnt;

        return NRF_SUCCESS;
}

static ret_code_t nrf_ble_whitelist_enable(void)
{
        ret_code_t ret;

        if (m_addr_cnt == 0)
        {
                return NRF_ERROR_DATA_SIZE;
        }

        for (uint32_t i = 0; i < BLE_GAP_WHITELIST_ADDR_MAX_COUNT; i++)
        {
                m_whitelist_addr_ptrs[i] = &m_whitelist_addrs[i];
        }

        ret = sd_ble_gap_whitelist_set(m_whitelist_addr_ptrs, m_addr_cnt);
        APP_ERROR_CHECK(ret);

        return NRF_SUCCESS;
}




ret_code_t scan_init(dm_ble_scan_t * const p_scan_ctx)
{
    VERIFY_PARAM_NOT_NULL(p_scan_ctx);

    /* We expect better performance when using extended advertising/scanning. However, for simplicity this example uses legacy scanning/advertising. */
#ifdef DM_USE_EXTENDED_SCANNING_ADVERTISING
    p_scan_ctx->scan_params.extended      = APP_SCAN_EXTENDED_ENABLED;
#else
    p_scan_ctx->scan_params.extended      = APP_SCAN_EXTENDED_DISABLED;
#endif
    p_scan_ctx->scan_params.active        = APP_SCAN_ACTIVE_DISABLED;
    p_scan_ctx->scan_params.interval      = APP_SCAN_SCAN_INTERVAL;
    p_scan_ctx->scan_params.window        = APP_SCAN_SCAN_WINDOW;
    p_scan_ctx->scan_params.filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL;
    p_scan_ctx->scan_params.timeout       = APP_SCAN_DURATION;
    p_scan_ctx->scan_params.scan_phys     = BLE_GAP_PHY_1MBPS;

    // Assign a buffer where the advertising reports are to be stored by the SoftDevice.
    p_scan_ctx->scan_buffer.p_data = p_scan_ctx->scan_buffer_data;
    p_scan_ctx->scan_buffer.len    = sizeof(p_scan_ctx->scan_buffer_data);

    return NRF_SUCCESS;
}

ret_code_t scan_start(dm_ble_scan_t const * const p_scan_ctx)
{
    VERIFY_PARAM_NOT_NULL(p_scan_ctx);

    ret_code_t err_code;

    // Start the scanning.
    err_code = sd_ble_gap_scan_start(&p_scan_ctx->scan_params, &p_scan_ctx->scan_buffer);

    // It is okay to ignore this error, because the scan stopped earlier.
    if ((err_code != NRF_ERROR_INVALID_STATE) && (err_code != NRF_SUCCESS))
    {
        NRF_LOG_ERROR("sd_ble_gap_scan_start returned 0x%x", err_code);
        return (err_code);
    }
    NRF_LOG_INFO("Starting scan.");

    return NRF_SUCCESS;
}




/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyse
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
        app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for initializing the timer module.
 */
static void timers_init(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    // Create the timer
    err_code = app_timer_create(&m_timer_id, APP_TIMER_MODE_REPEATED, time_update_handler);
    APP_ERROR_CHECK(err_code);

    // Start the timer with a 1-second interval
    err_code = app_timer_start(m_timer_id, APP_TIMER_TICKS(1000), NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for the GAP initialization.
 *
 * @details This function will set up all the necessary GAP (Generic Access Profile) parameters of
 *          the device. It also sets the permissions and appearance.
 */
static void gap_params_init(void)
{
        uint32_t err_code;
        ble_gap_conn_params_t gap_conn_params;
        ble_gap_conn_sec_mode_t sec_mode;

        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

        err_code = sd_ble_gap_device_name_set(&sec_mode,
                                              (const uint8_t *) DEVICE_NAME,
                                              strlen(DEVICE_NAME));
        APP_ERROR_CHECK(err_code);

        memset(&gap_conn_params, 0, sizeof(gap_conn_params));

        gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
        gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
        gap_conn_params.slave_latency     = SLAVE_LATENCY;
        gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

        err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
        APP_ERROR_CHECK(err_code);

        ble_gap_addr_t ble_address = {.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC,
                                      .addr_id_peer = 0,
                                      .addr = {0xC3, 0x11, 0x11, 0x11, 0x11, 0xFF}};
        err_code = sd_ble_gap_addr_set(&ble_address);

        //ble_gap_addr_t ble_address = {.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC,
        //                              .addr_id_peer = 0,
        //                              .addr = {0xC3, 0x22, 0x22, 0x22, 0x22, 0xFF}};
        //err_code = sd_ble_gap_addr_set(&ble_address);

        APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
        APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for handling the data from the Nordic UART Service.
 *
 * @details This function will process the data received from the Nordic UART BLE Service and send
 *          it to the UART module.
 *
 * @param[in] p_evt       Nordic UART Service event.
 */
/**@snippet [Handling the data received over BLE] */
static void nus_data_handler(ble_nus_evt_t * p_evt)
{

        if (p_evt->type == BLE_NUS_EVT_RX_DATA)
        {
                uint32_t err_code;

                NRF_LOG_DEBUG("Received data from BLE NUS. Writing data on UART.");
                NRF_LOG_HEXDUMP_DEBUG(p_evt->params.rx_data.p_data, p_evt->params.rx_data.length);

                for (uint32_t i = 0; i < p_evt->params.rx_data.length; i++)
                {
                        do
                        {
                                err_code = app_uart_put(p_evt->params.rx_data.p_data[i]);
                                if ((err_code != NRF_SUCCESS) && (err_code != NRF_ERROR_BUSY))
                                {
                                        NRF_LOG_ERROR("Failed receiving NUS message. Error 0x%x. ", err_code);
                                        APP_ERROR_CHECK(err_code);
                                }
                        } while (err_code == NRF_ERROR_BUSY);
                }
                if (p_evt->params.rx_data.p_data[p_evt->params.rx_data.length - 1] == '\r')
                {
                        while (app_uart_put('\n') == NRF_ERROR_BUSY);
                }
        }

}
/**@snippet [Handling the data received over BLE] */


/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
        uint32_t err_code;
        ble_nus_init_t nus_init;
        nrf_ble_qwr_init_t qwr_init = {0};

        // Initialize Queued Write Module.
        qwr_init.error_handler = nrf_qwr_error_handler;

        err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
        APP_ERROR_CHECK(err_code);

        // Initialize NUS.
        memset(&nus_init, 0, sizeof(nus_init));

        nus_init.data_handler = nus_data_handler;

        err_code = ble_nus_init(&m_nus, &nus_init);
        APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling an event from the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module
 *          which are passed to the application.
 *
 * @note All this function does is to disconnect. This could have been done by simply setting
 *       the disconnect_on_fail config parameter, but instead we use the event handler
 *       mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
        uint32_t err_code;

        if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
        {
                err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
                APP_ERROR_CHECK(err_code);
        }
}


/**@brief Function for handling errors from the Connection Parameters module.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
        APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
        uint32_t err_code;
        ble_conn_params_init_t cp_init;

        memset(&cp_init, 0, sizeof(cp_init));

        cp_init.p_conn_params                  = NULL;
        cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
        cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
        cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
        cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
        cp_init.disconnect_on_fail             = false;
        cp_init.evt_handler                    = on_conn_params_evt;
        cp_init.error_handler                  = conn_params_error_handler;

        err_code = ble_conn_params_init(&cp_init);
        APP_ERROR_CHECK(err_code);
}




/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
        uint32_t err_code;

        ble_gap_evt_t const * p_gap_evt = &p_ble_evt->evt.gap_evt;

        switch (p_ble_evt->header.evt_id)
        {
        case BLE_GAP_EVT_CONNECTED:
                NRF_LOG_INFO("Connected");
                m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
                err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
                APP_ERROR_CHECK(err_code);

                bsp_board_led_on(CONNECTED_LED);
                bsp_board_led_off(ADVERTISING_LED);

                m_advertising_is_running  = false;

                break;

        case BLE_GAP_EVT_DISCONNECTED:
                NRF_LOG_INFO("Disconnected");
                // LED indication will be changed when advertising starts.
                m_conn_handle = BLE_CONN_HANDLE_INVALID;
                bsp_board_led_off(CONNECTED_LED);
                m_advertising_is_running  = false;
                break;

        case BLE_GAP_EVT_SCAN_REQ_REPORT:
        {

                int i=0;
                bool is_match = false;
                ble_gap_evt_scan_req_report_t const * p_scan_req_report = (ble_gap_evt_scan_req_report_t *)&p_ble_evt->evt.gap_evt.params.scan_req_report;
                //NRF_LOG_INFO("BLE_GAP_EVT_SCAN_REQ_REPORT");
                //Get_Connect_MAC_Address((ble_gap_addr_t *)&p_gap_evt->params.scan_req_report.peer_addr);
                for (i=0; i < m_addr_cnt; i++)
                {
                        if (addr_cmp(&m_whitelist_addrs[i], (ble_gap_addr_t *)&p_gap_evt->params.scan_req_report.peer_addr) == true)
                        {
                                is_match = true;
                                break;
                        }
                }
                //NRF_LOG_INFO("BLE_GAP_EVT_SCAN_REQ_REPORT");
                //NRF_LOG_INFO("i = %d, is_match = %d, m_addr_cnt = %d", i, is_match, m_addr_cnt);
                if (is_match == false)
                {
                        if (m_addr_cnt < BLE_GAP_WHITELIST_ADDR_MAX_COUNT)
                        {
                                  //NRF_LOG_INFO("add whitelist: Addr cnt = %d, is_match = %d", m_addr_cnt, is_match);
                                  memcpy(&m_whitelist_addrs[m_addr_cnt], (ble_gap_addr_t *)&p_gap_evt->params.scan_req_report.peer_addr, sizeof(ble_gap_addr_t));
                                  m_addr_cnt++;
                        }

                        //memcpy(&whitelist_addrs, (ble_gap_addr_t *)&p_gap_evt->params.scan_req_report.peer_addr, sizeof(ble_gap_addr_t));

//                        if (i < BLE_GAP_WHITELIST_ADDR_MAX_COUNT-1)
//                        {
//                                ble_gap_addr_t whitelist_addrs;
//                                i = i+1;
//                                memcpy(&whitelist_addrs, (ble_gap_addr_t *)&p_gap_evt->params.scan_req_report.peer_addr, sizeof(ble_gap_addr_t));
//                                err_code = nrf_adv_add_whitelist(&whitelist_addrs, &i);
//                                APP_ERROR_CHECK(err_code);
//                        }
                }
        }
        break;

  case BLE_GAP_EVT_ADV_REPORT:
  {
      //NRF_LOG_INFO("BLE_GAP_EVT_ADV_REPORT event received");

      ble_gap_evt_adv_report_t const *p_adv_report = &p_ble_evt->evt.gap_evt.params.adv_report;
      //NRF_LOG_INFO("Received advertisement data with length: %d", p_adv_report->data.len);
      //NRF_LOG_HEXDUMP_INFO(p_adv_report->data.p_data, p_adv_report->data.len);

      if (m_logging_is_running)
      {
      // Filter for Apple Manufacturer Specific Data
      uint16_t data_offset = 0;
      uint8_t manuf_data_len = ble_advdata_search(p_adv_report->data.p_data,
                                                  p_adv_report->data.len,
                                                  &data_offset,
                                                  BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA);

      if (manuf_data_len >= 4) 
      {
          // Check if the manufacturer is Apple (0x004C)
          uint16_t company_identifier = uint16_decode(&p_adv_report->data.p_data[data_offset]);
          if (company_identifier == 0x004C)
          {
              if (p_adv_report->data.p_data[data_offset + 2] == 0x12)
              {


              uint8_t status_byte = p_adv_report->data.p_data[data_offset + 4];
              uint8_t flag_bits = status_byte & 0x30;


                if (flag_bits == 0x10)
                {
                  NRF_LOG_INFO("AirTag Identified");


                  //NRF_LOG_INFO("Full Advertisement Packet:");
                  //NRF_LOG_HEXDUMP_INFO(p_adv_report->data.p_data, p_adv_report->data.len);


                  //NRF_LOG_INFO("Manu spec data:");
                  //NRF_LOG_HEXDUMP_INFO(&p_adv_report->data.p_data[data_offset], manuf_data_len);

                  // Log the MAC address
                  Get_Connect_MAC_Address((ble_gap_addr_t *)&p_adv_report->peer_addr);

                  // Log the RSSI value
                  //NRF_LOG_INFO("RSSI: %d", p_adv_report->rssi);
                  char timestamp[16];
                  get_timestamp(timestamp, sizeof(timestamp));


                  NRF_LOG_INFO("RSSI: %d, timestamp: %u",
                               p_adv_report->rssi,
                               seconds_since_start);

                }
              }
          }
      }
      }

      // Resume the scanning.
      ret_code_t err_code = sd_ble_gap_scan_start(NULL, &m_scann_ctx.scan_buffer);
      if ((err_code != NRF_ERROR_INVALID_STATE) && (err_code != NRF_SUCCESS))
      {
          NRF_LOG_ERROR("sd_ble_gap_scan_start returned 0x%x", err_code);
      }

  }
break;



        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
                NRF_LOG_DEBUG("PHY update request.");
                ble_gap_phys_t const phys =
                {
                        .rx_phys = BLE_GAP_PHY_AUTO,
                        .tx_phys = BLE_GAP_PHY_AUTO,
                };
                err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
                APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
                // Pairing not supported
                err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
                APP_ERROR_CHECK(err_code);
                break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
                // No system attributes have been stored.
                err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
                APP_ERROR_CHECK(err_code);
                break;

        case BLE_GATTC_EVT_TIMEOUT:
                // Disconnect on GATT Client timeout event.
                err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                                 BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
                APP_ERROR_CHECK(err_code);
                break;

        case BLE_GATTS_EVT_TIMEOUT:
                // Disconnect on GATT Server timeout event.
                err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                                 BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
                APP_ERROR_CHECK(err_code);
                break;

        default:
                // No implementation needed.
                break;
        }
}

/**@brief SoftDevice SoC event handler.
 *
 * @param[in]   evt_id      SoC event.
 * @param[in]   p_context   Context.
 */
static void soc_evt_handler(uint32_t evt_id, void * p_context)
{
    switch (evt_id)
    {
        default:
            // No implementation needed.
            break;
    }
}
/**@brief Function for the SoftDevice initialization.
 *
 * @details This function initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
        ret_code_t err_code;

        err_code = nrf_sdh_enable_request();
        APP_ERROR_CHECK(err_code);

        // Configure the BLE stack using the default settings.
        // Fetch the start address of the application RAM.
        uint32_t ram_start = 0;
        err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
        APP_ERROR_CHECK(err_code);

        // Enable BLE stack.
        err_code = nrf_sdh_ble_enable(&ram_start);
        APP_ERROR_CHECK(err_code);

        err_code = sd_power_mode_set(NRF_POWER_MODE_CONSTLAT);
        APP_ERROR_CHECK(err_code);

        err_code = sd_power_dcdc_mode_set(NRF_POWER_DCDC_DISABLE);
        APP_ERROR_CHECK(err_code);

        // Register a handler for BLE events.
        NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, &m_scann_ctx);
        NRF_SDH_SOC_OBSERVER(m_soc_observer, APP_SOC_OBSERVER_PRIO, soc_evt_handler, NULL);
}


/**@brief Function for handling events from the GATT library. */
void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
        if ((m_conn_handle == p_evt->conn_handle) && (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED))
        {
                m_ble_nus_max_data_len = p_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
                NRF_LOG_INFO("Data len is set to 0x%X(%d)", m_ble_nus_max_data_len, m_ble_nus_max_data_len);
        }
        NRF_LOG_DEBUG("ATT MTU exchange completed. central 0x%x peripheral 0x%x",
                      p_gatt->att_mtu_desired_central,
                      p_gatt->att_mtu_desired_periph);
}


/**@brief Function for initializing the GATT library. */
void gatt_init(void)
{
        ret_code_t err_code;

        err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
        APP_ERROR_CHECK(err_code);

        err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
        APP_ERROR_CHECK(err_code);
}

/**@brief   Function for handling app_uart events.
 *
 * @details This function will receive a single character from the app_uart module and append it to
 *          a string. The string will be be sent over BLE when the last character received was a
 *          'new line' '\n' (hex 0x0A) or if the string has reached the maximum data length.
 */
/**@snippet [Handling the data received over UART] */
void uart_event_handle(app_uart_evt_t * p_event)
{
        static uint8_t data_array[BLE_NUS_MAX_DATA_LEN];
        static uint8_t index = 0;
        uint32_t err_code;

        switch (p_event->evt_type)
        {
        case APP_UART_DATA_READY:
                UNUSED_VARIABLE(app_uart_get(&data_array[index]));
                index++;

                if ((data_array[index - 1] == '\n') ||
                    (data_array[index - 1] == '\r') ||
                    (index >= m_ble_nus_max_data_len))
                {
                        if (index > 1)
                        {
                                NRF_LOG_DEBUG("Ready to send data over BLE NUS");
                                NRF_LOG_HEXDUMP_DEBUG(data_array, index);

                                do
                                {
                                        uint16_t length = (uint16_t)index;
                                        err_code = ble_nus_data_send(&m_nus, data_array, &length, m_conn_handle);
                                        if ((err_code != NRF_ERROR_INVALID_STATE) &&
                                            (err_code != NRF_ERROR_RESOURCES) &&
                                            (err_code != NRF_ERROR_NOT_FOUND))
                                        {
                                                APP_ERROR_CHECK(err_code);
                                        }
                                } while (err_code == NRF_ERROR_RESOURCES);
                        }

                        index = 0;
                }
                break;

        case APP_UART_COMMUNICATION_ERROR:
                APP_ERROR_HANDLER(p_event->data.error_communication);
                break;

        case APP_UART_FIFO_ERROR:
                APP_ERROR_HANDLER(p_event->data.error_code);
                break;

        default:
                break;
        }
}
/**@snippet [Handling the data received over UART] */


/**@brief  Function for initializing the UART module.
 */
/**@snippet [UART Initialization] */
static void uart_init(void)
{
        uint32_t err_code;
        app_uart_comm_params_t const comm_params =
        {
                .rx_pin_no    = RX_PIN_NUMBER,
                .tx_pin_no    = TX_PIN_NUMBER,
                .rts_pin_no   = RTS_PIN_NUMBER,
                .cts_pin_no   = CTS_PIN_NUMBER,
                .flow_control = APP_UART_FLOW_CONTROL_DISABLED,
                .use_parity   = false,
#if defined (UART_PRESENT)
                .baud_rate    = NRF_UART_BAUDRATE_115200
#else
                .baud_rate    = NRF_UARTE_BAUDRATE_115200
#endif
        };

        APP_UART_FIFO_INIT(&comm_params,
                           UART_RX_BUF_SIZE,
                           UART_TX_BUF_SIZE,
                           uart_event_handle,
                           APP_IRQ_PRIORITY_LOWEST,
                           err_code);
        APP_ERROR_CHECK(err_code);
}
/**@snippet [UART Initialization] */


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
        ret_code_t err_code;
        ble_advdata_t advdata;
        ble_advdata_t srdata;

        //ble_uuid_t adv_uuids[] = {{LBS_UUID_SERVICE, m_lbs.uuid_type}};

        NRF_LOG_INFO("Advertising init");

        // Build and set advertising data.
        memset(&advdata, 0, sizeof(advdata));

        advdata.name_type          = BLE_ADVDATA_FULL_NAME;
        advdata.include_appearance = false;
        advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

        memset(&srdata, 0, sizeof(srdata));

        err_code = ble_advdata_encode(&advdata, m_adv_data.adv_data.p_data, &m_adv_data.adv_data.len);
        APP_ERROR_CHECK(err_code);

        err_code = ble_advdata_encode(&srdata, m_adv_data.scan_rsp_data.p_data, &m_adv_data.scan_rsp_data.len);
        APP_ERROR_CHECK(err_code);

        ble_gap_adv_params_t adv_params;

        // Set advertising parameters.
        memset(&adv_params, 0, sizeof(adv_params));

        NRF_LOG_INFO("Set to 1 Mbps");
        adv_params.primary_phy     = BLE_GAP_PHY_1MBPS;
        adv_params.secondary_phy   = BLE_GAP_PHY_1MBPS;
        adv_params.properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;

        adv_params.duration        = APP_ADV_DURATION;
        adv_params.p_peer_addr     = NULL;
        adv_params.filter_policy   = BLE_GAP_ADV_FP_ANY;
        adv_params.interval        = APP_ADV_INTERVAL;


        adv_params.scan_req_notification = 1;

        err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, &adv_params);
        APP_ERROR_CHECK(err_code);

}





/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_with_filter_init(void)
{
        ret_code_t err_code;
        ble_advdata_t advdata;
        ble_advdata_t srdata;

        //ble_uuid_t adv_uuids[] = {{LBS_UUID_SERVICE, m_lbs.uuid_type}};

        NRF_LOG_INFO("Advertising init");

        // Build and set advertising data.
        memset(&advdata, 0, sizeof(advdata));

        advdata.name_type          = BLE_ADVDATA_FULL_NAME;
        advdata.include_appearance = false;
        advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

        memset(&srdata, 0, sizeof(srdata));

        err_code = ble_advdata_encode(&advdata, m_adv_data.adv_data.p_data, &m_adv_data.adv_data.len);
        APP_ERROR_CHECK(err_code);

        err_code = ble_advdata_encode(&srdata, m_adv_data.scan_rsp_data.p_data, &m_adv_data.scan_rsp_data.len);
        APP_ERROR_CHECK(err_code);

        ble_gap_adv_params_t adv_params;

        // Set advertising parameters.
        memset(&adv_params, 0, sizeof(adv_params));

        NRF_LOG_INFO("Set to 1 Mbps");
        adv_params.primary_phy     = BLE_GAP_PHY_1MBPS;
        adv_params.secondary_phy   = BLE_GAP_PHY_1MBPS;
        adv_params.properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;

        adv_params.duration        = APP_ADV_DURATION;
        adv_params.p_peer_addr     = NULL;
        adv_params.filter_policy   = BLE_GAP_ADV_FP_FILTER_BOTH;//BLE_GAP_ADV_FP_FILTER_SCANREQ;
        adv_params.interval        = APP_ADV_INTERVAL;
        adv_params.scan_req_notification = 1;

        nrf_ble_whitelist_enable();

        err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, &adv_params);

        APP_ERROR_CHECK(err_code);

}


/**@brief Function for starting advertising.
 */
static void advertising_start(void)
{
        ret_code_t err_code;

        if (m_advertising_is_running  == false)
        {
                err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);
                APP_ERROR_CHECK(err_code);
                m_advertising_is_running = true;
                NRF_LOG_INFO("Advertising Start");

                bsp_board_led_on(ADVERTISING_LED);
                bsp_board_led_off(CONNECTED_LED);
                m_advertising_filter_is_running = false;
        }
        else
        {
                advertising_stop();
                advertising_init();
                advertising_start();
                m_advertising_filter_is_running = false;
        }
}

/**@brief Function for starting advertising.
 */
static void advertising_with_filter_start(void)
{
        ret_code_t err_code;

        if (m_advertising_is_running  == false)
        {
                err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);
                APP_ERROR_CHECK(err_code);
                m_advertising_is_running = true;
                NRF_LOG_INFO("Advertising Start");

                bsp_board_led_on(ADVERTISING_LED);
                bsp_board_led_off(CONNECTED_LED);
        }
        else
        {
                advertising_stop();
                advertising_with_filter_init();
                advertising_start();
                m_advertising_filter_is_running = true;
        }

}



/**@brief Function for starting advertising.
 */
static void advertising_stop(void)
{
        ret_code_t err_code;
        if (m_advertising_is_running  == true)
        {
                err_code = sd_ble_gap_adv_stop(m_adv_handle);
                APP_ERROR_CHECK(err_code);
                m_advertising_is_running = false;
                NRF_LOG_INFO("Advertising Stop");
                bsp_board_led_off(ADVERTISING_LED);
                bsp_board_led_off(CONNECTED_LED);
        }
}

/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
        ret_code_t err_code = NRF_LOG_INIT(NULL);
        APP_ERROR_CHECK(err_code);

        NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
        ret_code_t err_code;
        err_code = nrf_pwr_mgmt_init();
        APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{
        if (NRF_LOG_PROCESS() == false)
        {
                nrf_pwr_mgmt_run();
        }
}

/**@brief Function for handling events from the button handler module.
 *
 * @param[in] pin_no        The pin that the event applies to.
 * @param[in] button_action The button action (press/release).
 */
static void button_event_handler(uint8_t pin_no, uint8_t button_action)
{
    switch (pin_no)
    {
    case ADV_FILTER_ON_BUTTON:  // Start logging
        if(button_action == APP_BUTTON_PUSH)
        {
            NRF_LOG_INFO("Current index: %d", current_distance_index);
            if (current_distance_index < (sizeof(distance_array) / sizeof(distance_array[0])) + 1 )
            {
                //float current_distance = distance_array[current_distance_index];
                //NRF_LOG_INFO("Starting logs %.1f m", current_distance);
                m_logging_is_running = true;
                if (!timer_started) {
                    seconds_since_start = 0;  // Reset the counter when logging starts
                    ret_code_t err_code = app_timer_start(m_timer_id, APP_TIMER_TICKS(1000), NULL);
                    APP_ERROR_CHECK(err_code);
                    timer_started = true;
                }
                current_distance_index++;
            }
            else
            {
                NRF_LOG_INFO("Experiment finished");
                m_logging_is_running = false;
                current_distance_index = 0;  // Reset for potential future use
            }
        }
        break;
    case ADV_FILTER_OFF_BUTTON:  // Repurposed to stop logging
        if(button_action == APP_BUTTON_PUSH)
        {
            NRF_LOG_INFO("Logging Stopped");
            m_logging_is_running = false;

        }
        break;

    case ADV_TURN_ON_BUTTON:  // Can be left unused or repurposed
        if(button_action == APP_BUTTON_PUSH)
        {
            // Potential future use or leave empty
        }
        break;

    case ADV_TURN_OFF_BUTTON:  // Can be left unused or repurposed
        if(button_action == APP_BUTTON_PUSH)
        {
            // Potential future use or leave empty
        }
        break;

    default:
        APP_ERROR_HANDLER(pin_no);
        break;
    }
}

/**@brief Function for initializing the button handler module.
 */
static void buttons_init(void)
{
        ret_code_t err_code;

        //The array must be static because a pointer to it will be saved in the button handler module.
        static app_button_cfg_t buttons[] =
        {
                {ADV_FILTER_ON_BUTTON,       false, BUTTON_PULL, button_event_handler},
                {ADV_FILTER_OFF_BUTTON,      false, BUTTON_PULL, button_event_handler},
                {ADV_TURN_ON_BUTTON,         false, BUTTON_PULL, button_event_handler},
                {ADV_TURN_OFF_BUTTON,        false, BUTTON_PULL, button_event_handler},
        };


        err_code = app_button_init(buttons, sizeof(buttons) / sizeof(buttons[0]),
                                   BUTTON_DETECTION_DELAY);
        APP_ERROR_CHECK(err_code);

        err_code = app_button_enable();
        APP_ERROR_CHECK(err_code);
}




/**@brief Application main function.
 */
int main(void)
{
        bool erase_bonds;

        // Initialize.
        uart_init();
        log_init();
        timers_init();
        buttons_init();
        bsp_board_init(BSP_INIT_LEDS);

        //buttons_leds_init(&erase_bonds);
        power_management_init();
        ble_stack_init();
        gap_params_init();
        gatt_init();
        services_init();
        advertising_init();
        conn_params_init();
        scan_init(&m_scann_ctx);

        // Start execution.
        //NRF_LOG_INFO("Advertising");
        advertising_start();
        scan_start(&m_scann_ctx);

        // Enter main loop.
        for (;;)
        {
                idle_state_handle();
        }
}


/**
 * @}
 */
