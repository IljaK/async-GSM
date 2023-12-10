#pragma once

constexpr unsigned long SOCKET_CLOSE_CONNECT_FAIL_TIMEOUT = 11000000ul;
#define GSM_SOCKET_ERROR_ID 255

enum GSM_SOCKET_STATE: uint8_t {
    GSM_SOCKET_STATE_CREATING,
    GSM_SOCKET_STATE_CONNECTING,
    GSM_SOCKET_STATE_READY,
    GSM_SOCKET_STATE_CLOSING,
    GSM_SOCKET_STATE_CLOSING_DELAY,
    GSM_SOCKET_STATE_DISCONNECTED
};

enum GSM_SOCKET_SSL: uint8_t {
    GSM_SOCKET_SSL_DISABLE,
    GSM_SOCKET_SSL_PROFILE_DEF,
    GSM_SOCKET_SSL_PROFILE_1,
    GSM_SOCKET_SSL_PROFILE_2,
    GSM_SOCKET_SSL_PROFILE_3,
    GSM_SOCKET_SSL_PROFILE_4
};

enum GSM_SOCKET_ERROR {
    GSM_SOCKET_ERROR_NONE,
    GSM_SOCKET_ERROR_NO_AVAILABLE_SLOTS,
    GSM_SOCKET_ERROR_FAILED_ACTIVATE_SERVICE,
    GSM_SOCKET_ERROR_FAILED_CREATE_SOCKET,
    GSM_SOCKET_ERROR_FAILED_CONFIGURE_SOCKET,
    GSM_SOCKET_ERROR_FAILED_CONNECT,
};