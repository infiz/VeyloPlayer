#pragma once

#include <QThread>

// The probe owns separate D3D devices, never LibVLC's rendering resources.
// Driver calls run away from the UI, including while Windows replaces a driver.
class GraphicsDeviceMonitor final : public QThread
{
    Q_OBJECT
public:
    using QThread::QThread;

signals:
    void deviceLost();
    void deviceRestored();

protected:
    void run() override;
};
