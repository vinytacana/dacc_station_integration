#ifndef CONFIG_DACC_ERROR_CODES_HPP
#define CONFIG_DACC_ERROR_CODES_HPP

namespace config_dacc {
namespace errors {

inline constexpr const char* OK = "ok";

inline constexpr const char* FEATURE_UNAVAILABLE = "feature_unavailable";

inline constexpr const char* AUDIO_SUBSYSTEM_MISSING = "audio_subsystem_missing";
inline constexpr const char* AUDIO_DEVICE_NOT_FOUND = "audio_device_not_found";
inline constexpr const char* AUDIO_NO_DEVICES = "audio_no_devices";
inline constexpr const char* AUDIO_SELECT_UNSUPPORTED = "audio_select_unsupported";
inline constexpr const char* AUDIO_SELECT_FAILED = "audio_select_failed";
inline constexpr const char* AUDIO_VOLUME_FAILED = "audio_volume_failed";
inline constexpr const char* AUDIO_MUTE_FAILED = "audio_mute_failed";

inline constexpr const char* DISPLAY_SUBSYSTEM_MISSING = "display_subsystem_missing";
inline constexpr const char* DISPLAY_NO_OUTPUTS = "display_no_outputs";
inline constexpr const char* DISPLAY_OUTPUT_NOT_FOUND = "display_output_not_found";
inline constexpr const char* DISPLAY_MODE_UNSUPPORTED = "display_mode_unsupported";
inline constexpr const char* DISPLAY_SESSION_UNKNOWN = "display_session_unknown";
inline constexpr const char* DISPLAY_SELECTION_UNSUPPORTED = "display_selection_unsupported";
inline constexpr const char* DISPLAY_SELECTION_FAILED = "display_selection_failed";
inline constexpr const char* DISPLAY_WINDOW_MOVE_FAILED = "display_window_move_failed";
inline constexpr const char* DISPLAY_SCALE_FAILED = "display_scale_failed";
inline constexpr const char* DISPLAY_RESOLUTION_FAILED = "display_resolution_failed";
inline constexpr const char* BRIGHTNESS_NOT_SUPPORTED = "brightness_not_supported";

inline constexpr const char* NETWORK_MANAGER_MISSING = "network_manager_missing";
inline constexpr const char* NETWORK_MANAGER_UNAVAILABLE = "network_manager_unavailable";
inline constexpr const char* WIFI_AUTH_FAILED = "wifi_auth_failed";
inline constexpr const char* WIFI_CONNECT_FAILED = "wifi_connect_failed";
inline constexpr const char* WIFI_DISABLED = "wifi_disabled";
inline constexpr const char* WIFI_DISCONNECT_FAILED = "wifi_disconnect_failed";
inline constexpr const char* WIFI_NO_NETWORKS = "wifi_no_networks";
inline constexpr const char* WIFI_NOT_FOUND = "wifi_not_found";
inline constexpr const char* WIFI_PASSWORD_REQUIRED = "wifi_password_required";
inline constexpr const char* WIFI_SCAN_FAILED = "wifi_scan_failed";
inline constexpr const char* WIFI_STATE_MISMATCH = "wifi_state_mismatch";
inline constexpr const char* WIFI_STATUS_FAILED = "wifi_status_failed";
inline constexpr const char* WIFI_TOGGLE_FAILED = "wifi_toggle_failed";

inline constexpr const char* BLUETOOTHCTL_MISSING = "bluetoothctl_missing";
inline constexpr const char* BLUETOOTHCTL_FAILED = "bluetoothctl_failed";
inline constexpr const char* BLUETOOTHCTL_SIGNALED = "bluetoothctl_signaled";
inline constexpr const char* BLUETOOTHCTL_TIMEOUT = "bluetoothctl_timeout";
inline constexpr const char* ADAPTER_UNAVAILABLE = "adapter_unavailable";
inline constexpr const char* ADAPTER_BLOCKED = "adapter_blocked";
inline constexpr const char* ADAPTER_SOFT_BLOCKED = "adapter_soft_blocked";
inline constexpr const char* ADAPTER_HARD_BLOCKED = "adapter_hard_blocked";
inline constexpr const char* DEVICE_UNAVAILABLE = "device_unavailable";
inline constexpr const char* DEVICE_NOT_CONNECTED = "device_not_connected";
inline constexpr const char* ALREADY_DONE = "already_done";
inline constexpr const char* AUTHENTICATION_FAILED = "authentication_failed";
inline constexpr const char* OPERATION_TIMEOUT = "operation_timeout";
inline constexpr const char* FORKPTY_FAILED = "forkpty_failed";
inline constexpr const char* WRITE_FAILED = "write_failed";
inline constexpr const char* READ_FAILED = "read_failed";
inline constexpr const char* RFKILL_UNBLOCK_FAILED = "rfkill_unblock_failed";
inline constexpr const char* STATE_MISMATCH = "state_mismatch";
inline constexpr const char* CONNECT_FAILED = "connect_failed";
inline constexpr const char* CONNECT_NOT_REFLECTED = "connect_not_reflected";
inline constexpr const char* DISCONNECT_FAILED = "disconnect_failed";
inline constexpr const char* DISCONNECT_NOT_REFLECTED = "disconnect_not_reflected";
inline constexpr const char* PAIR_FAILED = "pair_failed";
inline constexpr const char* PAIR_NOT_REFLECTED = "pair_not_reflected";
inline constexpr const char* TRUST_FAILED = "trust_failed";
inline constexpr const char* TRUST_NOT_REFLECTED = "trust_not_reflected";
inline constexpr const char* REMOVE_FAILED = "remove_failed";
inline constexpr const char* REMOVE_NOT_REFLECTED = "remove_not_reflected";
inline constexpr const char* STALE_PAIRING_REMOVE_FAILED = "stale_pairing_remove_failed";

} // namespace errors
} // namespace config_dacc

#endif
