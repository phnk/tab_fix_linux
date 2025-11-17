#include <gio/gio.h>
#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QtDBus/QtDBus>
#include <QKeyEvent>
#include <iostream>
#include <vector>

const char* HOTKEY_BUS_NAME = "org.phnk.TabFixHotkey";
const char* HOTKEY_OBJECT_PATH = "/org/phnk/TabFixHotkey";
const char* BUS_NAME = "org.phnk.TabFix";
const char* OBJECT_PATH = "/org/phnk/TabFix";
const char* INTERFACE_NAME = "org.phnk.TabFix";
const int TEXT_SIZE = 32;

struct WindowInfo {
    int id;
    std::string title;
    std::string className;
    std::string icon;
};

std::vector<WindowInfo> getWindows(GDBusConnection* conn) {
    std::vector<WindowInfo> windows;
    GError* error = nullptr;

    GVariant* result = g_dbus_connection_call_sync(
        conn,
        BUS_NAME,
        OBJECT_PATH,
        INTERFACE_NAME,
        "List",
        nullptr,
        G_VARIANT_TYPE("(a(isss))"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (error) {
        std::cerr << "Failed to call List: " << error->message << std::endl;
        g_error_free(error);
        g_object_unref(conn);
        return windows;
    }

    GVariantIter* iter;
    g_variant_get(result, "(a(isss))", &iter);

    gint index;
    gchar* title;
    gchar* wmClass;
    gchar* icon;

    while (g_variant_iter_loop(iter, "(isss)", &index, &title, &wmClass, &icon)) {
        if (strcmp(wmClass, "Gnome-shell") != 0) {
            WindowInfo w;
            w.id = index;
            w.title = title;
            w.className = wmClass;
            w.icon = icon;
            windows.push_back(w);
        }
    }

    g_variant_iter_free(iter);
    g_variant_unref(result);

    return windows;
}

// Minimal UI class
class UI : public QWidget {
public:
    UI() {
        setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setBrush(QColor{5, 22, 80});
        painter.drawRect(rect());
    }

    void keyPressEvent(QKeyEvent* event) override { 
        if (event->key() == Qt::Key_Escape) { 
            hide(); 
        } 
    };
};

// D-Bus adaptor for the hotkey
class HotkeyAdaptor : public QObject {
    Q_OBJECT
public:
    HotkeyAdaptor(UI* w, GDBusConnection* connection) 
            : window(w), connection(connection) {
        QDBusConnection bus = QDBusConnection::sessionBus();
        if (!bus.registerService(HOTKEY_BUS_NAME)) {
            std::cerr << "Failed to register hotkey D-Bus service. Maybe another instance is running?\n";
        }
        bus.registerObject(HOTKEY_OBJECT_PATH, this, QDBusConnection::ExportAllSlots);
    }

public slots:
    void ShowWindow() {

        // get all windows
        auto windows = getWindows(connection);

        int clientW = 1200;
        const int clientH = 40 * size(windows) + 10;
        window->resize(clientW, clientH);


        int iconSize = TEXT_SIZE;
        int iconX = 5;
        int y = 10;
        int deltaHeight = 8;
        int textX = iconX + iconSize + 10;


        // create the ui
        for (const auto& w : windows) { 
            std::cout << "Index: " << w.id << "\n"; 
            std::cout << " Title: " << w.title << "\n"; 
            std::cout << " Class: " << w.className << "\n"; 
            std::cout << " Icon: " << w.icon << "\n"; 
            std::cout << "---\n"; 

            y += iconSize + deltaHeight;
        }

        // Show the UI window
        window->show();
        window->raise();
        window->activateWindow();

    }

private:
    UI* window;
    GDBusConnection* connection;
};

int main(int argc, char** argv) {
    GError *error = nullptr;
    GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
    if (error) {
        std::cerr << "Failed to connect to D-Bus: " << error->message << std::endl;
        g_error_free(error);
        return 1;
    }

    QApplication app(argc, argv);
    UI window;

    // Create the hotkey D-Bus adaptor
    HotkeyAdaptor hotkey(&window, conn);

    int ret = app.exec();
    g_object_unref(conn);

    return ret;
}

#include "main.moc"
