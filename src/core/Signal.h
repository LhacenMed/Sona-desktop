#pragma once

#include <functional>
#include <utility>
#include <vector>

// Ends a Signal connection when destroyed, so a callback never outlives the object it captures.
class Subscription
{
public:
    Subscription() = default;
    explicit Subscription(std::function<void()> disconnect) : m_disconnect(std::move(disconnect)) {}
    Subscription(Subscription &&other) noexcept : m_disconnect(std::exchange(other.m_disconnect, nullptr)) {}
    Subscription &operator=(Subscription &&other) noexcept
    {
        if (this != &other) {
            reset();
            m_disconnect = std::exchange(other.m_disconnect, nullptr);
        }
        return *this;
    }
    ~Subscription() { reset(); }

private:
    void reset()
    {
        if (m_disconnect)
            std::exchange(m_disconnect, nullptr)();
    }

    std::function<void()> m_disconnect;
};

// Callbacks notified when something changes. The signal must outlive its subscriptions.
template <typename... Args>
class Signal
{
public:
    Signal() = default;
    Signal(const Signal &) = delete;
    Signal &operator=(const Signal &) = delete;

    [[nodiscard]] Subscription connect(std::function<void(Args...)> slot)
    {
        const int id = ++m_lastId;
        m_slots.emplace_back(id, std::move(slot));
        return Subscription([this, id] { std::erase_if(m_slots, [id](const auto &entry) { return entry.first == id; }); });
    }

    void emit(Args... args) const
    {
        // Iterates a copy, so callbacks may connect or disconnect while being notified.
        const auto slots = m_slots;
        for (const auto &[id, slot] : slots)
            slot(args...);
    }

private:
    std::vector<std::pair<int, std::function<void(Args...)>>> m_slots;
    int m_lastId = 0;
};
