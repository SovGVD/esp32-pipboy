#pragma once
#include "Arduino.h"
#include "SPI.h"
#include <algorithm>
#include <map>
#include <memory>
#include <string>
constexpr int CARD_NONE = 0;
struct Node {
    std::string path;
    bool directory = false;
    std::vector<uint8_t> bytes;
    std::vector<std::shared_ptr<Node>> children;
};
class File {
public:
    File() = default;
    explicit File(std::shared_ptr<Node> node) : node_(std::move(node)) {}
    explicit operator bool() const { return !!node_; }
    bool isDirectory() const { return node_ && node_->directory; }
    uint32_t size() const { return node_ ? node_->bytes.size() : 0; }
    bool seek(uint32_t offset) { pos_ = offset; return offset <= size(); }
    size_t read(uint8_t* bytes, size_t count) {
        count = std::min<size_t>(count, size() - pos_);
        std::memcpy(bytes, node_->bytes.data() + pos_, count);
        pos_ += count;
        return count;
    }
    const char* path() const { return node_->path.c_str(); }
    const char* name() const { return node_->path.c_str() + node_->path.find_last_of('/') + 1; }
    File openNextFile() {
        if (!node_ || child_ == node_->children.size()) return {};
        return File(node_->children[child_++]);
    }
    void close() { node_.reset(); }
private:
    std::shared_ptr<Node> node_;
    size_t pos_ = 0, child_ = 0;
};
struct SDClass {
    std::atomic<bool> mounted{false};
    std::mutex guard;
    std::map<std::string, std::vector<uint8_t>> files;
    bool begin(int, SPIClass&, unsigned) { return mounted; }
    int cardType() { return mounted ? 1 : CARD_NONE; }
    void end() {}
    void add(std::string path, std::vector<uint8_t> bytes = {0, 1, 2}) {
        std::lock_guard<std::mutex> lock(guard); files[std::move(path)] = std::move(bytes);
    }
    void clear() { std::lock_guard<std::mutex> lock(guard); files.clear(); }
    File open(const char*) {
        std::lock_guard<std::mutex> lock(guard);
        std::map<std::string, std::shared_ptr<Node>> nodes;
        auto root = std::make_shared<Node>(); root->path = "/"; root->directory = true;
        nodes["/"] = root;
        for (const auto& entry : files) {
            auto parent = root;
            size_t slash = entry.first.find('/', 1);
            while (slash != std::string::npos) {
                const auto path = entry.first.substr(0, slash);
                auto& node = nodes[path];
                if (!node) {
                    node = std::make_shared<Node>(); node->path = path; node->directory = true;
                    parent->children.push_back(node);
                }
                parent = node;
                slash = entry.first.find('/', slash + 1);
            }
            auto file = std::make_shared<Node>(); file->path = entry.first; file->bytes = entry.second;
            parent->children.push_back(file);
        }
        return File(root);
    }
};
inline SDClass SD;
