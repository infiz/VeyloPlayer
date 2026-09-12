#pragma once

namespace veylo {

// Ten consecutive half-second probes prevent restarting between the multiple
// device resets that can occur in a single driver installation.
class GraphicsRecovery
{
public:
    enum Change { Unchanged, Lost, Restored };

    Change sample(bool healthy)
    {
        if (!healthy) {
            healthySamples_ = 0;
            if (waiting_) return Unchanged;
            waiting_ = true;
            return Lost;
        }
        if (!waiting_ || ++healthySamples_ < 10) return Unchanged;
        waiting_ = false;
        healthySamples_ = 0;
        return Restored;
    }

private:
    bool waiting_ = false;
    int healthySamples_ = 0;
};

} // namespace veylo
