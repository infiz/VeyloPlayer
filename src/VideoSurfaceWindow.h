#pragma once

#include <QTimer>
#include <QWindow>
#include <QPoint>

class QMouseEvent;

class VideoSurfaceWindow final : public QWindow
{
    Q_OBJECT

public:
    explicit VideoSurfaceWindow(QWindow *parent = nullptr);

signals:
    void clicked();
    void doubleClicked();
    void pointerActivity();

protected:
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#endif
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QTimer singleClickTimer_;
    QTimer pointerPollTimer_;
    QPoint lastGlobalPointerPosition_;
    bool suppressNextRelease_ = false;
};
