#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <vector>
#include <algorithm>
#include <iostream>

using namespace emscripten;

// Linear Probing Implementation
class LinearProbing {
private:
    std::vector<int> table;
    int size;
    const int EMPTY = -1;
    const int DELETED = -2;

public:
    LinearProbing(int s) : size(s) {
        table.assign(size, EMPTY);
        updateVisualization();
    }

    void clear() {
        table.assign(size, EMPTY);
        updateVisualization();
    }

    int hash(int key) {
        return key % size;
    }

    bool insert(int key) {
        // Check if already exists
        if (searchInternal(key) != -1) return false;

        val probeSteps = val::array();
        int h = hash(key);
        int start = h;
        
        // Linear Probe for empty slot
        while (table[h] != EMPTY && table[h] != DELETED) {
            val step = val::object();
            step.set("index", h);
            step.set("type", "collision");
            step.set("existingValue", table[h]);
            probeSteps.call<void>("push", step);

            h = (h + 1) % size;
            if (h == start) return false; // Table is full
        }

        val step = val::object();
        step.set("index", h);
        step.set("type", "insert");
        probeSteps.call<void>("push", step);

        table[h] = key;
        updateVisualizationWithAnimation(probeSteps);
        return true;
    }

    int searchInternal(int key) {
        int h = hash(key);
        int start = h;

        while (table[h] != EMPTY) {
            if (table[h] == key) return h;
            h = (h + 1) % size;
            if (h == start) break;
        }
        return -1;
    }

    void search(int key) {
        int idx = searchInternal(key);
        if (idx != -1) {
             val::global("highlightItem").call<void>("call", val::undefined(), idx); // Pass index to highlight
        } else {
            // Not found
        }
    }

    void remove(int key) {
        int idx = searchInternal(key);
        if (idx != -1) {
            table[idx] = DELETED; // Lazy deletion
            updateVisualization();
        }
    }

    void updateVisualization() {
        val js_table = val::array();
        for (int val : table) {
            if (val == EMPTY) js_table.call<void>("push", val::null());
            else if (val == DELETED) js_table.call<void>("push", std::string("DEL")); 
            else js_table.call<void>("push", val);
        }

        val data = val::object();
        data.set("table", js_table);
        data.set("size", size);

        val::global("renderHashMap").call<void>("call", val::undefined(), data);
    }

    void updateVisualizationWithAnimation(val probeSteps) {
        val js_table = val::array();
        for (int val : table) {
            if (val == EMPTY) js_table.call<void>("push", val::null());
            else if (val == DELETED) js_table.call<void>("push", std::string("DEL")); 
            else js_table.call<void>("push", val);
        }

        val data = val::object();
        data.set("table", js_table);
        data.set("size", size);
        data.set("steps", probeSteps);

        val::global("animateHashMap").call<void>("call", val::undefined(), data);
    }
};

LinearProbing* hashMap = nullptr;

extern "C" void initHashMap(int size) {
    if (hashMap) delete hashMap;
    hashMap = new LinearProbing(size);
}

extern "C" bool insertHashMap(int value) {
    if (!hashMap) initHashMap(20);
    return hashMap->insert(value);
}

extern "C" void deleteHashMap(int value) {
    if (!hashMap) initHashMap(20);
    hashMap->remove(value);
}

extern "C" void searchHashMap(int value) {
    if (!hashMap) initHashMap(20);
    hashMap->search(value);
}

extern "C" void clearHashMap() {
    if (!hashMap) initHashMap(20);
    hashMap->clear();
}

EMSCRIPTEN_BINDINGS(hashmap_module) {
    function("initHashMap", &initHashMap);
    function("insertHashMap", &insertHashMap);
    function("deleteHashMap", &deleteHashMap);
    function("searchHashMap", &searchHashMap);
    function("clearHashMap", &clearHashMap);
}
