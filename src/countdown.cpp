// SPDX-FileCopyrightText: 2026 Andy Stewart
// SPDX-License-Identifier: GPL-3.0-only

#include "countdown.h"

#include <LayerShellQt/window.h>
#include <QEventLoop>
#include <QGuiApplication>
#include <QQuickItem>
#include <QQuickView>
#include <QTimer>
#include <QtGui/qguiapplication_platform.h>

#include <wayland-client.h>

namespace {
bool synchronizeCompositor() {
  if (!QGuiApplication::platformName().startsWith(QStringLiteral("wayland")))
    return true;
  auto *native = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();
  if (!native)
    return false;
  struct Sync {
    QEventLoop loop;
    bool done = false;
  } sync;
  static const wl_callback_listener listener = {
      [](void *data, wl_callback *, uint32_t) {
        auto &sync = *static_cast<Sync *>(data);
        sync.done = true;
        sync.loop.quit();
      }};
  auto *callback = wl_display_sync(native->display());
  if (!callback)
    return false;
  wl_callback_add_listener(callback, &listener, &sync);
  wl_display_flush(native->display());
  QTimer::singleShot(2000, &sync.loop, &QEventLoop::quit);
  sync.loop.exec();
  wl_callback_destroy(callback);
  return sync.done;
}
}

bool showCaptureCountdown(int seconds, QString *error) {
  QQuickView view;
  view.setResizeMode(QQuickView::SizeRootObjectToView);
  view.setColor(Qt::transparent);
  view.setFlags(Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus |
                Qt::WindowTransparentForInput);
  view.setInitialProperties({{QStringLiteral("seconds"), seconds}});

  if (QGuiApplication::platformName().startsWith(QStringLiteral("wayland"))) {
    auto *layer = LayerShellQt::Window::get(&view);
    layer->setScope(QStringLiteral("omarchy-screenshot-countdown"));
    layer->setLayer(LayerShellQt::Window::LayerOverlay);
    layer->setAnchors(LayerShellQt::Window::AnchorNone);
    layer->setExclusiveZone(-1);
    layer->setKeyboardInteractivity(
        LayerShellQt::Window::KeyboardInteractivityNone);
    layer->setActivateOnShow(false);
    layer->setWantsToBeOnActiveScreen(true);
  }

  view.setSource(
      QUrl(QStringLiteral("qrc:/qt/qml/OmarchyScreenshot/Countdown.qml")));
  if (view.status() != QQuickView::Ready) {
    *error = QStringLiteral("Cannot load Countdown.qml");
    return false;
  }

  const bool quitOnClose = QGuiApplication::quitOnLastWindowClosed();
  QGuiApplication::setQuitOnLastWindowClosed(false);
  QEventLoop loop;
  bool started = false;
  QTimer timeout;
  timeout.setSingleShot(true);
  QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
  QObject::connect(view.rootObject(), SIGNAL(finished()), &loop, SLOT(quit()));
  const auto startConnection = QObject::connect(
      &view, &QQuickWindow::frameSwapped, &loop, [&] {
        if (started)
          return;
        started = true;
        // Start at the first visible frame, so loading the QML does not
        // shorten the time available to prepare a window or menu.
        timeout.start(seconds * 1000 + 3000);
        QMetaObject::invokeMethod(view.rootObject(), "start");
      }, Qt::QueuedConnection);
  timeout.start(3000);
  view.show();
  loop.exec();
  QObject::disconnect(startConnection);
  timeout.stop();
  bool completed = started && view.rootObject()->opacity() == 0;

  if (completed) {
    // Animation completion precedes rendering. Submit two fully transparent
    // frames before unmapping, so even a compositor's exit animation snapshots
    // an empty buffer rather than the last visible countdown number.
    QEventLoop cleared;
    int transparentFrames = 0;
    const auto clearConnection = QObject::connect(
        &view, &QQuickWindow::frameSwapped, &cleared, [&] {
          if (++transparentFrames == 2)
            cleared.quit();
          else
            view.update();
        }, Qt::QueuedConnection);
    QTimer::singleShot(2000, &cleared, &QEventLoop::quit);
    view.update();
    cleared.exec();
    QObject::disconnect(clearConnection);
    completed = transparentFrames >= 2;
    QGuiApplication::sync();
  }

  view.hide();
  view.destroy();
  QGuiApplication::sync();
  // The capture worker uses a separate Wayland connection. Wait for the server
  // to acknowledge the unmap on this connection before that worker can start.
  completed = synchronizeCompositor() && completed;
  QGuiApplication::setQuitOnLastWindowClosed(quitOnClose);
  if (!completed)
    *error = QStringLiteral("Countdown did not finish rendering; capture cancelled.");
  return completed;
}
