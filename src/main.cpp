#include <gio/gio.h>
#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QPainter>
#include <QtDBus/QtDBus>
#include <QKeyEvent>
#include <iostream>
#include <vector>
#include <QIcon>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <unordered_map>
#include <map>
#include <cstdlib>
#include <QFile>
#include <QStandardPaths>
#include <QDir>

const char* HOTKEY_BUS_NAME   = "org.phnk.TabFixHotkey";
const char* HOTKEY_OBJECT_PATH= "/org/phnk/TabFixHotkey";
const char* BUS_NAME          = "org.phnk.TabFix";
const char* OBJECT_PATH       = "/org/phnk/TabFix";
const char* INTERFACE_NAME    = "org.phnk.TabFix";
const int TEXT_SIZE           = 32;

struct WindowInfo {
    int id;
    std::string title;
    std::string className;
    std::string icon;
};

QPixmap loadWindowIcon(const QStringList &iconNames, const QString &fallbackName = ":/icons/default_app.png", int size = 32) {
    QPixmap pix;

    for (const QString &name : iconNames) {
        QIcon icon = QIcon::fromTheme(name);
        if (!icon.isNull()) {
            pix = icon.pixmap(size, size);
            if (!pix.isNull()) return pix;
        }
    }

    for (const QString &dir : QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
        for (const QString &name : iconNames) {
            QString path = QDir(dir).filePath(QString("icons/hicolor/48x48/apps/%1.png").arg(name));
            if (QFile::exists(path) && pix.load(path)) return pix;
        }
    }

    pix.load(fallbackName);
    return pix;
}

bool activateWindow(GDBusConnection* conn, int index) {
    GError* error = nullptr;
    GVariant* params = g_variant_new("(i)", index);

    GVariant* result = g_dbus_connection_call_sync(
        conn,
        BUS_NAME,
        OBJECT_PATH,
        INTERFACE_NAME,
        "Activate",
        params,
        G_VARIANT_TYPE("(s)"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (error) {
        std::cerr << "Failed to call Activate: " << error->message << std::endl;
        g_error_free(error);
        return false;
    }

    const char* status;
    g_variant_get(result, "(&s)", &status);
    g_variant_unref(result);

    return std::string(status) == "OK";
}

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

class UI : public QWidget {
    Q_OBJECT
public:
    UI(GDBusConnection* c) : conn(c) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    }

    void populateWindows(const std::vector<WindowInfo>& windows) {
        clearLayout();
        keyToWindowIndex.clear();
        std::map<QChar,int> firstCharCounts;

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(10, 10, 10, 10);
        mainLayout->setSpacing(8);

        int iconSize = TEXT_SIZE;

        for (const auto &w : windows) {
            if (w.className.empty()) continue;

            QString className = QString::fromStdString(w.className);
            QChar firstChar = className[0].toLower();
            int count = firstCharCounts[firstChar]++;
            QChar suffix = QChar('a' + count);

            std::string key;
            key += firstChar.toLatin1();
            key += suffix.toLatin1();
            keyToWindowIndex[key] = w.id;

            QString labelText = QString("%1%2 | %3 | %4")
                .arg(firstChar)
                .arg(suffix)
                .arg(QString::fromStdString(w.title))
                .arg(className);

            QWidget* row = new QWidget(this);
            QHBoxLayout* rowLayout = new QHBoxLayout(row);
            rowLayout->setContentsMargins(0,0,0,0);
            rowLayout->setSpacing(10);

            // Icon
            QStringList iconParts = QString::fromStdString(w.icon).split(",");
            QPixmap pix = loadWindowIcon(iconParts, ":/icons/default_app.png", iconSize);
            QLabel* iconLabel = new QLabel(row);
            iconLabel->setPixmap(pix);
            iconLabel->setFixedSize(iconSize, iconSize);
            rowLayout->addWidget(iconLabel);

            // Text
            QLabel* textLabel = new QLabel(labelText, row);
            textLabel->setStyleSheet("color: white; font-size: 32px; font-weight: bold; font-family: monospace;");
            rowLayout->addWidget(textLabel);

            mainLayout->addWidget(row);
        }

        setLayout(mainLayout);
        showCentered();
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
            clearLayout();
            buffer.clear();
        } else {
            buffer += event->text().toLower().toStdString();
            auto it = keyToWindowIndex.find(buffer);
            if (it != keyToWindowIndex.end()) {
                int index = it->second;
                hide();
                buffer.clear();
                if (conn) ::activateWindow(conn, index);
            } else if (buffer.length() >= 2) {
                buffer.clear();
            }
        }
    }

private:
    void showCentered() {
        adjustSize(); // recalc size based on layout
        setFixedSize(size()); // optional: prevent resizing
        QRect screen = QGuiApplication::primaryScreen()->geometry();
        move(screen.center() - rect().center()); // use rect(), not frameGeometry()
        show();
        raise();
        activateWindow();
    }

    void clearLayout() {
        if (layout()) {
            QLayoutItem* item;
            while ((item = layout()->takeAt(0)) != nullptr) {
                delete item->widget();
                delete item;
            }
            delete layout();
        }
    }

public:
    std::unordered_map<std::string,int> keyToWindowIndex;

private:
    std::string buffer;
    GDBusConnection* conn = nullptr;
};

// ---- D-Bus adaptor ----
class HotkeyAdaptor : public QObject {
    Q_OBJECT
public:
    HotkeyAdaptor(UI* w, GDBusConnection* c) : window(w), connection(c) {
        QDBusConnection bus = QDBusConnection::sessionBus();
        if (!bus.registerService(HOTKEY_BUS_NAME)) {
            std::cerr << "Failed to register hotkey D-Bus service\n";
        }
        bus.registerObject(HOTKEY_OBJECT_PATH, this, QDBusConnection::ExportAllSlots);
    }

public slots:
    void ShowWindow() {
        if (!window->isVisible()) {
            auto windows = getWindows(connection);
            window->populateWindows(windows);
        } else {
            window->raise();
            window->activateWindow();
        }
    }

private:
    UI* window;
    GDBusConnection* connection;
};

int main(int argc, char** argv) {
    GError* error = nullptr;
    GDBusConnection* conn = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
    if (error) {
        std::cerr << "Failed to connect to D-Bus: " << error->message << std::endl;
        g_error_free(error);
        return 1;
    }

    QApplication app(argc, argv);
    UI window(conn);

    HotkeyAdaptor hotkey(&window, conn);

    int ret = app.exec();
    g_object_unref(conn);
    return ret;
}

#include "main.moc"
