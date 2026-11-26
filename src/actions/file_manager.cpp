#include "file_manager.h"

#include <dbus/dbus.h>

#include <cstdio>

namespace {

/**
 * Percent-encodes a filesystem path into a file:// URI, leaving '/' and
 * unreserved characters bare so the result stays readable.
 * @param path absolute filesystem path
 * @return a file:// URI for that path
 */
std::string toFileUri(const std::string& path) {
    std::string uri = "file://";
    for (unsigned char c : path) {
        bool unreserved = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
                           c == '-' || c == '.' || c == '_' || c == '~' || c == '/';
        if (unreserved) {
            uri += static_cast<char>(c);
        } else {
            char buf[4];
            std::snprintf(buf, sizeof(buf), "%%%02X", c);
            uri += buf;
        }
    }
    return uri;
}

}  // namespace

/**
 * Asks the user's default file manager to show and select a file, via the
 * freedesktop org.freedesktop.FileManager1.ShowItems D-Bus interface. Any
 * compliant file manager (Dolphin, Nautilus, Nemo, ...) implements this,
 * so it respects whatever is actually configured as default, instead of
 * shelling out to one specific binary. Fire-and-forget: no reply awaited.
 * @param path absolute path to the file to reveal and select
 */
void showInFileManager(const std::string& path) {
    if (path.empty()) return;

    DBusError err;
    dbus_error_init(&err);

    DBusConnection* conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (!conn) {
        dbus_error_free(&err);
        return;
    }

    DBusMessage* msg = dbus_message_new_method_call("org.freedesktop.FileManager1", "/org/freedesktop/FileManager1",
                                                      "org.freedesktop.FileManager1", "ShowItems");
    if (!msg) return;

    DBusMessageIter iter;
    DBusMessageIter arrayIter;
    dbus_message_iter_init_append(msg, &iter);
    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &arrayIter);

    std::string uri = toFileUri(path);
    const char* uriCStr = uri.c_str();
    dbus_message_iter_append_basic(&arrayIter, DBUS_TYPE_STRING, &uriCStr);
    dbus_message_iter_close_container(&iter, &arrayIter);

    const char* startupId = "";
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &startupId);

    dbus_connection_send(conn, msg, nullptr);
    dbus_connection_flush(conn);
    dbus_message_unref(msg);
}
