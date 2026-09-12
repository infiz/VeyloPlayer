#include "VideoSurfaceWindow.h"

#include <QCursor>
#include <QGuiApplication>
#include <QMouseEvent>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

VideoSurfaceWindow::VideoSurfaceWindow(QWindow *parent)
    : QWindow(parent)
{
    setFlags(Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);
    setTitle(QStringLiteral("VeyloPlayer video surface"));
    setIcon(QGuiApplication::windowIcon());
    singleClickTimer_.setSingleShot(true);
    singleClickTimer_.setInterval(250);
    connect(&singleClickTimer_, &QTimer::timeout, this, &VideoSurfaceWindow::clicked);
    pointerPollTimer_.setInterval(100);
    lastGlobalPointerPosition_ = QCursor::pos();
    connect(&pointerPollTimer_, &QTimer::timeout, this, [this] {
        const QPoint position = QCursor::pos();
        if (position == lastGlobalPointerPosition_) {
            return;
        }
        lastGlobalPointerPosition_ = position;
        if (!isVisible()) {
            return;
        }
        const QRect globalBounds(mapToGlobal(QPoint(0, 0)), size());
        if (globalBounds.contains(position)) {
            emit pointerActivity();
        }
    });
    pointerPollTimer_.start();
    create();
}

#ifdef Q_OS_WIN
bool VideoSurfaceWindow::nativeEvent(const QByteArray &eventType, void *message,
                                    qintptr *result)
{
    const auto *nativeMessage = static_cast<MSG *>(message);
    if (nativeMessage->message == WM_PAINT) {
        // Qt does not paint this plain QWindow. Cover the loading surface
        // until LibVLC's child window is ready to display video.
        PAINTSTRUCT paint;
        const HDC context = BeginPaint(nativeMessage->hwnd, &paint);
        FillRect(context, &paint.rcPaint,
                 static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        EndPaint(nativeMessage->hwnd, &paint);
        *result = 0;
        return true;
    }
    if (nativeMessage->message == WM_ERASEBKGND) {
        // This native window sits above the QML canvas. Paint its uncovered
        // background black while LibVLC is creating or replacing its output.
        RECT bounds;
        GetClientRect(nativeMessage->hwnd, &bounds);
        FillRect(reinterpret_cast<HDC>(nativeMessage->wParam), &bounds,
                 static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        *result = 1;
        return true;
    }
    return QWindow::nativeEvent(eventType, message, result);
}
#endif

void VideoSurfaceWindow::mouseMoveEvent(QMouseEvent *event)
{
    emit pointerActivity();
    QWindow::mouseMoveEvent(event);
}

void VideoSurfaceWindow::mousePressEvent(QMouseEvent *event)
{
    emit pointerActivity();
    QWindow::mousePressEvent(event);
}

void VideoSurfaceWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit pointerActivity();
        if (suppressNextRelease_) {
            suppressNextRelease_ = false;
        } else {
            singleClickTimer_.start();
        }
    }
    QWindow::mouseReleaseEvent(event);
}

void VideoSurfaceWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        singleClickTimer_.stop();
        suppressNextRelease_ = true;
        emit pointerActivity();
        emit doubleClicked();
    }
    QWindow::mouseDoubleClickEvent(event);
}
