#include<emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h> // Required for calling JavaScript
using namespace emscripten;
// --- C++ Data Structure Logic ---
struct Node {
    int data;
    Node* next;

    Node(int val) : data(val), next(nullptr) {}
};

Node* head = nullptr;
int node_count = 0; // To help with positioning

// Function to add a node and then trigger a JavaScript drawing function
extern "C" void addNode(int value) {
    Node* newNode = new Node(value);
    newNode->next = head;
    head = newNode;
    
    // Calculate visualization position (simple layout)
    int x_pos = 100 + (node_count * 120);
    int y_pos = 150;
    
    // Increment count for the next node
    node_count++;

    // Crucial step: Call the JavaScript function 'drawNewNode'
    // This tells the frontend to update the visualization.
    emscripten::val::global("drawNewNode").call<void>("call", 
        emscripten::val::undefined(), // 'this' context (not needed)
        emscripten::val(value), 
        emscripten::val(x_pos), 
        emscripten::val(y_pos),
        emscripten::val(node_count)
    );
}

// Function to get the current count of nodes
extern "C" int getNodeCount() {
    return node_count;
}
// --------------------------------

// --- Embind Wrapper ---
using namespace emscripten;

EMSCRIPTEN_BINDINGS(linked_list_module) {
    function("addNode", &addNode);
    function("getNodeCount", &getNodeCount);
}