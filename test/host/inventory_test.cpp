#include <Inventory.h>
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "NativeFrame.h"

class Frame : public NativeFrame<inventory::Canvas> {
public:
    void header(size_t selectedCategory) {
        // The existing main/submenu renderer positions characters individually
        // at 10 px. Reproduce that chrome; body rendering below is actual code.
        font = &monofonto11pt7b;
        const char* main[] = {"STAT", "INV", "DATA", "MAP", "RADIO"};
        const int positions[] = {60, 140, 210, 290, 360};
        for (unsigned i = 0; i < 5; ++i) fixedText(positions[i], 21, main[i], inventory::ink);
        line(0,27,133,27,inventory::ink); line(133,27,133,11,inventory::ink);
        line(134,11,137,11,inventory::ink); line(173,11,177,11,inventory::ink);
        line(177,11,177,27,inventory::ink); line(177,27,479,27,inventory::ink);
        const char* labels[] = {"WEAPONS", "APPAREL", "AID", "MISC", "JUNK"};
        const int offsets[] = {0,81,162,203,254};
        strict = false; // Original scrolling submenu intentionally runs offscreen.
        for (unsigned i = 0; i < 5; ++i)
            fixedText(140+offsets[i]-offsets[selectedCategory],51,labels[i], i==selectedCategory?inventory::ink:inventory::dim);
        strict = true;
        font = &monofonto10pt7b;
    }
};

int main(int argc, char** argv) {
    std::set<std::string> ids;
    std::set<std::vector<uint16_t>> silhouettes;
    unsigned count = 0;
    for (size_t c = 0; c < inventory::categoryCount; ++c) {
        const auto* category = inventory::category(c);
        assert(category && category->count == 6);
        assert(inventory::clampSelection(*category,65535) == category->count-1);
        for (uint16_t i = 0; i < category->count; ++i) {
            const auto& item = category->items[i];
            assert(ids.insert(item.id).second);
            assert(item.model.commands && item.model.count);
            Frame model;
            inventory::drawModel(model,item.model,234,101,234,88);
            const auto illuminated = std::count(model.pixels.begin(),model.pixels.end(),inventory::ink);
            assert(illuminated > 120); // No blank/vanishing vector assets.
            assert(silhouettes.insert(model.pixels).second); // Every item has distinct art.
            Frame native;
            native.header(c);
            inventory::render(native,c,i,true);
            assert(native.texts > 8 && native.lines > 10 && native.triangles > 0);
            if (argc == 2) native.save(std::string(argv[1])+"/"+item.id+".ppm");
            Frame navigation;
            inventory::render(navigation,c,i,false);
            assert(native.pixels != navigation.pixels);
            ++count;
        }
        Frame stale;
        inventory::render(stale,c,65535,true);
        Frame finalItem;
        inventory::render(finalItem,c,category->count-1,true);
        assert(stale.pixels == finalItem.pixels);
    }
    assert(!inventory::category(inventory::categoryCount));
    Frame invalid;
    inventory::render(invalid,9999,65535,true);
    assert(invalid.texts == 1);
    Frame degenerate;
    inventory::drawModel(degenerate,inventory::category(0)->items[0].model,0,0,0,0);
    assert(degenerate.lines == 0 && degenerate.triangles == 0);
    assert(count == 30 && ids.size() == 30);
    std::cout << "Inventory: 30 unique vector models, all categories/selections, native text/pixel bounds and stale-index checks passed\n";
}
