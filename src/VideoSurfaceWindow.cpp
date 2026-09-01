#include "VideoSurfaceWindow.h"

#include <QCursor>
#include <QGuiApplication>
#include <QMouseEvent>

VideoSurfaceWindow::VideoSurfaceWindow(QWindow *parent)
    : QWindow(parent)
{
    setFlags(Qt::FramelessWindowHint);
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
