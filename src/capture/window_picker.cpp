#include "window_picker.h"

#include <dbus/dbus.h>

#include <cstdint>
#include <string>

namespace {

/**
 * Walks the a{sv} dict returned by KWin's queryWindowInfo, pulling out the
 * "pid" (int32) and "desktopFile" (string) entries.
 * @param dictIter iterator positioned at the start of the dict's entries
 * @return a PickedWindow with whatever of pid/desktopFile was present; pid
 *         stays 0 if the key was missing (an empty map on cancel)
 */
PickedWindow extractPickedWindow(DBusMessageIter& dictIter) {
    PickedWindow result;
    while (dbus_message_iter_get_arg_type(&dictIter) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry;
        dbus_message_iter_recurse(&dictIter, &entry);

        const char* key = nullptr;
        dbus_message_iter_get_basic(&entry, &key);
        dbus_message_iter_next(&entry);

        DBusMessageIter variant;
        dbus_message_iter_recurse(&entry, &variant);

        if (key && std::string(key) == "pid") {
            int32_t pid = 0;
            dbus_message_iter_get_basic(&variant, &pid);
            result.pid = pid;
        } else if (key && std::string(key) == "desktopFile") {
            const char* desktopFile = nullptr;
            dbus_message_iter_get_basic(&variant, &desktopFile);
            if (desktopFile) result.desktopFile = desktopFile;
        }

        dbus_message_iter_next(&dictIter);
    }
    return result;
}

}  // namespace

/**
 * Calls org.kde.KWin's queryWindowInfo over the session bus. KWin switches
 * the cursor to a crosshair and blocks the call until the user clicks a
 * window or cancels with Escape (an empty map comes back on cancel).
 * @return the clicked window's pid/desktopFile, or nullopt on cancel or any D-Bus error
 */
std::optional<PickedWindow> pickWindow() {
    DBusError err;
    dbus_error_init(&err);

    DBusConnection* conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (!conn) {
        dbus_error_free(&err);
        return std::nullopt;
    }

    DBusMessage* msg = dbus_message_new_method_call("org.kde.KWin", "/KWin", "org.kde.KWin", "queryWindowInfo");
    if (!msg) return std::nullopt;

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(conn, msg, DBUS_TIMEOUT_INFINITE, &err);
    dbus_message_unref(msg);
    if (!reply) {
        dbus_error_free(&err);
        return std::nullopt;
    }

    DBusMessageIter args;
    if (!dbus_message_iter_init(reply, &args)) {
        dbus_message_unref(reply);
        return std::nullopt;
    }

    DBusMessageIter dict;
    dbus_message_iter_recurse(&args, &dict);
    PickedWindow result = extractPickedWindow(dict);

    dbus_message_unref(reply);
    if (result.pid == 0) return std::nullopt;
    return result;
}
