#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <vector>
#include <algorithm>
#include <iostream>

using namespace emscripten;

// --- C++ Heap Logic ---
std::vector<int> heap;
bool isMinHeap = false; // Default to Max Heap

// Helper to pass the heap data to JavaScript
void updateVisualization() {
    std::cout << "C++: updateVisualization called. Heap size: " << heap.size() << std::endl;
    val js_heap = val::array();
    for (size_t i = 0; i < heap.size(); ++i) {
        js_heap.call<void>("push", heap[i]);
    }
    
    // Call the global JavaScript function 'renderHeap'
    val::global("renderHeap").call<void>("call", val::undefined(), js_heap);
}

// Comparison helper
bool shouldSwap(int parentVal, int childVal) {
    if (isMinHeap) {
        return childVal < parentVal; // Min Heap: Child smaller than parent -> Swap
    } else {
        return childVal > parentVal; // Max Heap: Child larger than parent -> Swap
    }
}

void bubbleUp(int index) {
    if (index == 0) return;
    int parentIndex = (index - 1) / 2;
    
    if (shouldSwap(heap[parentIndex], heap[index])) {
        std::swap(heap[index], heap[parentIndex]);
        bubbleUp(parentIndex);
    }
}

void bubbleDown(int index) {
    int leftChild = 2 * index + 1;
    int rightChild = 2 * index + 2;
    int target = index;

    // Find the "target" child to swap with (smallest for MinHeap, largest for MaxHeap)
    if (leftChild < heap.size()) {
        if (shouldSwap(heap[target], heap[leftChild])) {
            target = leftChild;
        }
    }
    
    if (rightChild < heap.size()) {
        if (shouldSwap(heap[target], heap[rightChild])) {
            target = rightChild;
        }
    }

    if (target != index) {
        std::swap(heap[index], heap[target]);
        bubbleDown(target);
    }
}

// Re-build the entire heap (used when toggling type)
void rebuildHeap() {
    // Standard "heapify" algorithm: start from last non-leaf node and bubble down
    for (int i = (heap.size() / 2) - 1; i >= 0; i--) {
        bubbleDown(i);
    }
    updateVisualization();
}

extern "C" void insertHeap(int value) {
    std::cout << "C++: insertHeap called with value " << value << std::endl;
    heap.push_back(value);
    bubbleUp(heap.size() - 1);
    updateVisualization();
}

extern "C" void extractRoot() {
    if (heap.empty()) return;
    
    heap[0] = heap.back();
    heap.pop_back();
    if (!heap.empty()) {
        bubbleDown(0);
    }
    updateVisualization();
}

extern "C" void clearHeap() {
    heap.clear();
    updateVisualization();
}

extern "C" void toggleHeapType(bool makeMinHeap) {
    isMinHeap = makeMinHeap;
    rebuildHeap();
}

// --- Embind Wrapper ---
EMSCRIPTEN_BINDINGS(heap_module) {
    function("insertHeap", &insertHeap);
    function("extractRoot", &extractRoot);
    function("clearHeap", &clearHeap);
    function("toggleHeapType", &toggleHeapType);
}
