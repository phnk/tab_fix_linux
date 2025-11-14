#include <iostream>
#include <gio/gio.h>
#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QKeyEvent>

int main() {
    GError *error = nullptr;

    // Connect to session bus
    GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
    if (error) {
        std::cerr << "Failed to connect to D-Bus: " << error->message << std::endl;
        g_error_free(error);
        return 1;
    }

    // Call the List method
    GVariant *result = g_dbus_connection_call_sync(
        connection,
        "org.phnk.TabFix",           // bus name
        "/org/phnk/TabFix",          // object path
        "org.phnk.TabFix",           // interface name
        "List",                      // method name
        nullptr,                     // parameters (none)
        G_VARIANT_TYPE("(a(isss))"), // expected return type
        G_DBUS_CALL_FLAGS_NONE,
        -1,                          // default timeout
        nullptr,
        &error
    );

    if (error) {
        std::cerr << "Failed to call List: " << error->message << std::endl;
        g_error_free(error);
        g_object_unref(connection);
        return 1;
    }

    // Parse the result
    GVariantIter *iter;
    g_variant_get(result, "(a(isss))", &iter);

    gint index;
    gchar *title, *wmClass, *icon;

    std::cout << "Windows:\n";
    std::cout << "========\n";

    while (g_variant_iter_loop(iter, "(isss)", &index, &title, &wmClass, &icon)) {
        std::cout << "Index: " << index << "\n";
        std::cout << "  Title: " << title << "\n";
        std::cout << "  Class: " << wmClass << "\n";
        std::cout << "  Icon: " << (strlen(icon) > 50 ? "[base64 data]" : icon) << "\n";
        std::cout << "---\n";
    }

    g_variant_iter_free(iter);
    g_variant_unref(result);
    g_object_unref(connection);

    return 0;
}
