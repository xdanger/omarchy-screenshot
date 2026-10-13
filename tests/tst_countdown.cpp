// SPDX-FileCopyrightText: 2026 Andy Stewart
// SPDX-License-Identifier: GPL-3.0-only

#include "countdown.h"

#include <QDir>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QPointer>
#include <QQuickItem>
#include <QQuickView>
#include <QSet>
#include <QTest>
#include <QTimer>

class CountdownTest : public QObject {
  Q_OBJECT

private slots:
  void fadesAndUnmapsBeforeCapture() {
    QSet<int> numbers;
    QImage preview;
    bool transparentFrame = false;
    bool inputPassesThrough = false;
    QPointer<QQuickView> observed;
    QObject frameObserver;
    QTimer sample;
    sample.setInterval(25);
    connect(&sample, &QTimer::timeout, this, [&] {
      for (auto *window : QGuiApplication::allWindows()) {
        auto *view = qobject_cast<QQuickView *>(window);
        if (!view || !view->rootObject() || !view->isVisible())
          continue;
        auto *root = view->rootObject();
        numbers.insert(root->property("remaining").toInt());
        // The window becomes visible before its first frame is rendered.
        // Sample the animated dial after rendering has started, rather than
        // treating a startup buffer as the countdown preview.
        if (preview.isNull() && root->property("elapsed").toDouble() >= 0.25) {
          const QImage frame = view->grabWindow();
          if (!frame.isNull() &&
              frame.pixelColor(frame.width() / 2, frame.height() / 2).alpha() > 0)
            preview = frame;
        }
        if (observed)
          continue;
        observed = view;
        inputPassesThrough =
            view->flags().testFlag(Qt::WindowTransparentForInput) &&
            view->flags().testFlag(Qt::WindowDoesNotAcceptFocus);
        const QPointer<QQuickView> guard(view);
        connect(view, &QQuickWindow::frameSwapped, &frameObserver, [&, guard] {
          if (!guard || guard->rootObject()->opacity() != 0 || transparentFrame)
            return;
          const QImage image = guard->grabWindow();
          if (image.isNull())
            return;
          for (int y = 0; y < image.height(); ++y)
            for (int x = 0; x < image.width(); ++x)
              if (image.pixelColor(x, y).alpha() != 0)
                return;
          transparentFrame = true;
        }, Qt::QueuedConnection);
      }
    });
    sample.start();
    QElapsedTimer elapsed;
    elapsed.start();
    QString error;
    const bool finished = showCaptureCountdown(5, &error);
    sample.stop();
    QVERIFY2(finished, qPrintable(error));
    QVERIFY(elapsed.elapsed() >= 5000);
    QVERIFY(elapsed.elapsed() < 9000);
    QCOMPARE(numbers, (QSet<int>{5, 4, 3, 2, 1}));
    QVERIFY(inputPassesThrough);
    QVERIFY(transparentFrame);
    QVERIFY(!preview.isNull());
    QCOMPARE(preview.pixelColor(0, 0).alpha(), 0);
    QVERIFY(preview.pixelColor(preview.width() / 2, preview.height() / 2).alpha() > 0);
    QVERIFY(observed.isNull());
    for (auto *window : QGuiApplication::allWindows())
      QVERIFY(!window->isVisible());
    const QString artifacts = qEnvironmentVariable("OMARCHY_SCREENSHOT_TEST_ARTIFACT_DIR");
    if (!artifacts.isEmpty()) {
      QVERIFY(QDir().mkpath(artifacts));
      QVERIFY(preview.save(artifacts + QStringLiteral("/countdown.png")));
    }
  }
};

QTEST_MAIN(CountdownTest)
#include "tst_countdown.moc"
