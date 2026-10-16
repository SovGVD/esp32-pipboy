#pragma once
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

constexpr int pdTRUE = 1, pdPASS = 1;
constexpr uint32_t portMAX_DELAY = UINT32_MAX;
namespace host {
struct EndTask {};
inline std::atomic<bool> stop{false};
inline std::atomic<uint32_t> timeOffset{0};
inline std::thread task;
inline const auto start = std::chrono::steady_clock::now();
}
inline uint32_t millis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - host::start).count() + host::timeOffset;
}
struct SerialStub {
    void println(const char*) {}
    void printf(const char*, ...) {}
};
inline SerialStub Serial;
using SemaphoreHandle_t = std::mutex*;
inline SemaphoreHandle_t xSemaphoreCreateMutex() { return new std::mutex; }
inline void xSemaphoreTake(SemaphoreHandle_t mutex, uint32_t) { mutex->lock(); }
inline void xSemaphoreGive(SemaphoreHandle_t mutex) { mutex->unlock(); }
struct Queue {
    std::mutex guard;
    size_t capacity, size;
    std::deque<std::vector<uint8_t>> values;
    Queue(size_t capacity, size_t size) : capacity(capacity), size(size) {}
};
using QueueHandle_t = Queue*;
inline QueueHandle_t xQueueCreate(size_t capacity, size_t size) { return new Queue(capacity, size); }
inline int xQueueSend(Queue* queue, const void* value, uint32_t) {
    std::lock_guard<std::mutex> lock(queue->guard);
    if (queue->values.size() == queue->capacity) return 0;
    const auto* bytes = static_cast<const uint8_t*>(value);
    queue->values.emplace_back(bytes, bytes + queue->size);
    return pdTRUE;
}
inline int xQueueReceive(Queue* queue, void* value, uint32_t) {
    std::lock_guard<std::mutex> lock(queue->guard);
    if (queue->values.empty()) return 0;
    std::memcpy(value, queue->values.front().data(), queue->size);
    queue->values.pop_front();
    return pdTRUE;
}
inline void vQueueDelete(Queue* queue) { delete queue; }
inline void vTaskDelay(unsigned ticks) {
    if (host::stop) throw host::EndTask{};
    std::this_thread::sleep_for(std::chrono::milliseconds(ticks));
}
inline void vTaskDelete(void*) { throw host::EndTask{}; }
inline int xTaskCreatePinnedToCore(void (*function)(void*), const char*, unsigned, void*,
                                    unsigned, void*, unsigned) {
    host::task = std::thread([=] { try { function(nullptr); } catch (const host::EndTask&) {} });
    return pdPASS;
}
