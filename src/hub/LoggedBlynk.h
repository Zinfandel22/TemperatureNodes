#pragma once
#include <map>

template <typename Client, typename Output>
class LoggedBlynk {
public:
    LoggedBlynk(Client &client, Output &output) : client_(client), output_(output), buffering_(false) {}

    void beginGroup() {
        buffering_ = true;
        buffer_.clear();
        // Delegate to Blynk's protocol-level grouping
        client_.beginGroup();
    }

    template <typename Value>
    void virtualWrite(int pin, const Value &value) {
        // Always collect in buffer for reference
        if (buffering_) {
            buffer_[pin] = valueToString(value);
        }
        // Forward to Blynk client immediately (protocol handles grouping)
        client_.virtualWrite(pin, value);
    }

    void endGroup() {
        if (buffering_) {
            int count = buffer_.size();
            if (count > 0) {
                output_.printf("[Blynk] Batch: %d datastreams with shared timestamp\n", count);
            }
            buffer_.clear();
        }
        buffering_ = false;
        // Send end-of-group marker
        client_.endGroup();
    }

    template <typename Message>
    void logEvent(const char *event, const Message &message) {
        output_.print("[Blynk] Event ");
        output_.print(event);
        output_.print(" = ");
        output_.println(message);
        client_.logEvent(event, message);
    }

private:
    String valueToString(float value) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.3f", value);
        return String(buf);
    }

    String valueToString(int value) {
        return String(value);
    }

    String valueToString(unsigned int value) {
        return String(value);
    }

    template <typename Value>
    String valueToString(const Value &value) {
        return String(value);
    }

    Client &client_;
    Output &output_;
    bool buffering_;
    std::map<int, String> buffer_;
};
