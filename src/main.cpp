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
#include <QTimer>
#include <algorithm>
#include <QGraphicsDropShadowEffect>

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

    QStringList namesToTry;
    for (const QString &name : iconNames) {
        namesToTry << name;

        QString lowerName = name.toLower();
        if (lowerName == "discord") {
            namesToTry << "com.discordapp.Discord";
        } else if (lowerName == "slack") {
            namesToTry << "com.slack.Slack";
        } else if (lowerName == "spotify") {
            namesToTry << "com.spotify.Client";
        }
    }

    for (const QString &name : namesToTry) {
        QIcon icon = QIcon::fromTheme(name);
        if (!icon.isNull()) {
            pix = icon.pixmap(size, size);
            if (!pix.isNull()) return pix;
        }
    }

    QStringList searchPaths = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    QString home = QDir::homePath();
    searchPaths << home + "/.local/share/flatpak/exports/share";
    searchPaths << "/var/lib/flatpak/exports/share";

    QStringList iconSizes = {"48x48", "256x256", "128x128", "scalable"};

    for (const QString &dir : searchPaths) {
        for (const QString &iconSize : iconSizes) {
            for (const QString &name : namesToTry) {
                QString path = QDir(dir).filePath(QString("icons/hicolor/%1/apps/%2.png").arg(iconSize, name));
                if (QFile::exists(path) && pix.load(path)) {
                    return pix.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                }
            }
        }
    }

    QStringList appDirs = {"/usr/share", home + "/.local/share"};
    for (const QString &baseDir : appDirs) {
        for (const QString &name : namesToTry) {
            QString path = QString("%1/%2/%3.png").arg(baseDir, name, name);
            if (QFile::exists(path) && pix.load(path)) {
                return pix.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }

            path = QString("%1/pixmaps/%2.png").arg(baseDir, name);
            if (QFile::exists(path) && pix.load(path)) {
                return pix.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
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

    std::sort(windows.begin(), windows.end(), [](const WindowInfo &a, const WindowInfo &b) {
        return a.title < b.title;
    });

    g_variant_iter_free(iter);
    g_variant_unref(result);

    return windows;
}

class UI : public QWidget {
    Q_OBJECT
public:
    UI(GDBusConnection* c) : conn(c) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
        setAttribute(Qt::WA_TranslucentBackground);

        QVBoxLayout* outerLayout = new QVBoxLayout(this);
        outerLayout->setContentsMargins(25, 25, 25, 25);
        outerLayout->setSpacing(0);

        frame = new QFrame(this);
        frame->setMaximumWidth(1200);
        frame->setObjectName("mainFrame");
        frame->setStyleSheet(
            "QFrame#mainFrame { "
            "  background-color: rgb(5,22,80); "
            "}");
        frame->setFrameShape(QFrame::NoFrame);

        rowsLayout = new QVBoxLayout(frame);
        rowsLayout->setContentsMargins(10, 10, 10, 10);
        rowsLayout->setSpacing(8);

        shadow = new QGraphicsDropShadowEffect(frame);
        shadow->setBlurRadius(40);
        shadow->setOffset(0, 0);
        shadow->setColor(QColor(255, 80, 80, 200));
        shadow->setEnabled(false);
        setGraphicsEffect(shadow);

        outerLayout->addWidget(frame);
        setLayout(outerLayout);
    }

    void flashError() {
        shadow->setEnabled(true);
        shadow->setBlurRadius(40);
        shadow->setColor(QColor(255, 80, 80, 255));

        QTimer::singleShot(200, this, [this]() {
            shadow->setEnabled(false);
        });
    }

    void populateWindows(const std::vector<WindowInfo>& windows) {
        clearRows();
        keyToWindowIndex.clear();

        std::map<QChar,int> firstCharCounts;
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

            QWidget* row = new QWidget(frame);
            QHBoxLayout* rowLayout = new QHBoxLayout(row);
            rowLayout->setContentsMargins(0,0,0,0);
            rowLayout->setSpacing(10);

            QStringList iconParts = QString::fromStdString(w.icon).split(",");
            QPixmap pix = loadWindowIcon(iconParts, ":/icons/default_app.png", iconSize);
            QLabel* iconLabel = new QLabel(row);
            iconLabel->setPixmap(pix);
            iconLabel->setFixedSize(iconSize, iconSize);
            rowLayout->addWidget(iconLabel);

            QLabel* textLabel = new QLabel(labelText, row);
            textLabel->setStyleSheet("color: white; font-size: 32px; font-weight: bold; font-family: monospace;");
            rowLayout->addWidget(textLabel);

            rowsLayout->addWidget(row);
        }

        frame->adjustSize();
        adjustSize();
        showCentered();
    }

protected:
    void keyPressEvent(QKeyEvent* event) override {
        if (event->key() == Qt::Key_Escape) {
            hide();
            clearRows();
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
                flashError();
            }
        }
    }

private:
    void showCentered() {
        adjustSize();
        setFixedSize(size());
        QRect screen = QGuiApplication::primaryScreen()->geometry();
        move(screen.center() - rect().center());
        if (!isVisible()) show();
        raise();
        activateWindow();
    }

    void clearRows() {
        if (!rowsLayout) return;
        QLayoutItem* item;
        while ((item = rowsLayout->takeAt(0)) != nullptr) {
            if (QWidget* w = item->widget()) {
                w->hide();
                delete w;
            }
            delete item;
        }
    }

public:
    std::unordered_map<std::string,int> keyToWindowIndex;
    QFrame* frame;

private:
    QVBoxLayout* rowsLayout = nullptr;
    std::string buffer;
    GDBusConnection* conn = nullptr;
    QGraphicsDropShadowEffect* shadow = nullptr;
};

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
