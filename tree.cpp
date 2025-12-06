#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <iostream>
#include <vector>
#include <string>

using namespace emscripten;

struct Node {
    int data;
    Node* left;
    Node* right;
    int id; // Unique ID for D3
    int height;

    Node(int val, int nodeId) : data(val), left(nullptr), right(nullptr), id(nodeId), height(1) {}
};

Node* bstRoot = nullptr;
int nextId = 0;

// Helper to serialize tree to JS object
// Helper to serialize tree to JS object
// Helper to serialize tree to JS object - Forward declaration
val getTreeData(Node* node);

// Helper to log events to JS
void logEvent(std::string type, val data, std::string message) {
    val::global("handleEvent").call<void>("call", val::undefined(), type, data, message);
}

void updateBSTVisualization() {
    val treeData = getTreeData(bstRoot);
    logEvent("snapshot", treeData, "Tree Updated");
}

// --- AVL Helpers ---
bool useAVL = false;

int getHeight(Node* N) {
    if (N == nullptr) return 0;
    return N->height;
}

int max(int a, int b) {
    return (a > b) ? a : b;
}

Node* rightRotate(Node* y) {
    // Highlight nodes involved
    val ids = val::array();
    ids.call<void>("push", y->id);
    if (y->left) ids.call<void>("push", y->left->id);
    logEvent("highlight", ids, "Right Rotating...");

    Node* x = y->left;
    Node* T2 = x->right;

    // Perform rotation
    x->right = y;
    y->left = T2;

    // Update heights
    y->height = max(getHeight(y->left), getHeight(y->right)) + 1;
    x->height = max(getHeight(x->left), getHeight(x->right)) + 1;

    // Snapshot after rotation
    // Snapshot removed to avoid broken tree state
    // logEvent("snapshot", getTreeData(bstRoot), "Rotated");

    return x;
}

Node* leftRotate(Node* x) {
    // Highlight nodes involved
    val ids = val::array();
    ids.call<void>("push", x->id);
    if (x->right) ids.call<void>("push", x->right->id);
    logEvent("highlight", ids, "Left Rotating...");

    Node* y = x->right;
    Node* T2 = y->left;

    // Perform rotation
    y->left = x;
    x->right = T2;

    // Update heights
    x->height = max(getHeight(x->left), getHeight(x->right)) + 1;
    y->height = max(getHeight(y->left), getHeight(y->right)) + 1;

    // Snapshot after rotation
    // Snapshot removed to avoid broken tree state
    // logEvent("snapshot", getTreeData(bstRoot), "Rotated");

    return y;
}

int getBalance(Node* N) {
    if (N == nullptr) return 0;
    return getHeight(N->left) - getHeight(N->right);
}

// Helper to serialize tree to JS object
val getTreeData(Node* node) {
    if (!node) return val::null();

    val obj = val::object();
    obj.set("id", node->id);
    obj.set("value", node->data);
    obj.set("height", node->height);
    obj.set("balanceFactor", getBalance(node));
    
    val children = val::array();
    if (node->left) children.call<void>("push", getTreeData(node->left));
    if (node->right) children.call<void>("push", getTreeData(node->right));
    
    if (node->left || node->right) {
        obj.set("children", children);
    }

    return obj;
}

void inorderExtraction(Node* root, std::vector<int>& nodes) {
    if (!root) return;
    inorderExtraction(root->left, nodes);
    nodes.push_back(root->data);
    inorderExtraction(root->right, nodes);
}

void deleteTree(Node* node) {
    if (!node) return;
    deleteTree(node->left);
    deleteTree(node->right);
    delete node;
}

extern "C" void insertBST(int value); // Forward declaration
Node* insertRec(Node* node, int value); // Forward declaration

void rebalanceBST() {
    std::vector<int> nodes;
    inorderExtraction(bstRoot, nodes);
    
    // Clear current tree
    deleteTree(bstRoot);
    bstRoot = nullptr;
    nextId = 0;
    
    // Re-insert with AVL enabled
    // We need to reset root and nextId, but we want to visualize the reconstruction?
    // Or just show the final result? The user wants "rebalance".
    // Let's just re-insert them one by one.
    
    // Note: insertBST calls updateBSTVisualization which logs a snapshot.
    // This will create an animation of the tree being rebuilt balanced.
    for (int val : nodes) {
        bstRoot = insertRec(bstRoot, val);
        updateBSTVisualization();
    }
}

extern "C" void setAVL(bool enable) {
    useAVL = enable;
    if (useAVL) {
        rebalanceBST();
    }
}

// --- BST Operations ---

Node* insertRec(Node* node, int value) {
    if (!node) {
        return new Node(value, nextId++);
    }
    if (value < node->data) {
        node->left = insertRec(node->left, value);
        if (useAVL) logEvent("snapshot", getTreeData(bstRoot), "Rebalancing...");
    } else if (value > node->data) {
        node->right = insertRec(node->right, value);
        if (useAVL) logEvent("snapshot", getTreeData(bstRoot), "Rebalancing...");
    } else {
        return node; // Duplicate keys not allowed
    }

    if (!useAVL) return node;

    // Update height
    node->height = 1 + max(getHeight(node->left), getHeight(node->right));

    // Get balance factor
    int balance = getBalance(node);

    // Left Left Case
    if (balance > 1 && value < node->left->data)
        return rightRotate(node);

    // Right Right Case
    if (balance < -1 && value > node->right->data)
        return leftRotate(node);

    // Left Right Case
    if (balance > 1 && value > node->left->data) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }

    // Right Left Case
    if (balance < -1 && value < node->right->data) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    return node;
}

extern "C" void insertBST(int value) {
    bstRoot = insertRec(bstRoot, value);
    updateBSTVisualization();
}

Node* minValueNode(Node* node) {
    Node* current = node;
    while (current && current->left != nullptr)
        current = current->left;
    return current;
}

Node* deleteRec(Node* root, int value) {
    if (!root) return root;

    if (value < root->data) {
        root->left = deleteRec(root->left, value);
        if (useAVL) logEvent("snapshot", getTreeData(bstRoot), "Rebalancing...");
    } else if (value > root->data) {
        root->right = deleteRec(root->right, value);
        if (useAVL) logEvent("snapshot", getTreeData(bstRoot), "Rebalancing...");
    } else {
        if (!root->left) {
            Node* temp = root->right;
            delete root;
            return temp;
        } else if (!root->right) {
            Node* temp = root->left;
            delete root;
            return temp;
        }

        Node* temp = minValueNode(root->right);
        root->data = temp->data;
        root->right = deleteRec(root->right, temp->data);
        if (useAVL) logEvent("snapshot", getTreeData(bstRoot), "Rebalancing...");
    }
    
    if (!useAVL) return root;
    if (!root) return root; // Should not happen if we returned earlier, but safe check

    // Update height
    root->height = 1 + max(getHeight(root->left), getHeight(root->right));

    // Get balance factor
    int balance = getBalance(root);

    // Left Left Case
    if (balance > 1 && getBalance(root->left) >= 0)
        return rightRotate(root);

    // Left Right Case
    if (balance > 1 && getBalance(root->left) < 0) {
        root->left = leftRotate(root->left);
        return rightRotate(root);
    }

    // Right Right Case
    if (balance < -1 && getBalance(root->right) <= 0)
        return leftRotate(root);

    // Right Left Case
    if (balance < -1 && getBalance(root->right) > 0) {
        root->right = rightRotate(root->right);
        return leftRotate(root);
    }

    return root;
}

extern "C" void deleteBST(int value) {
    bstRoot = deleteRec(bstRoot, value);
    updateBSTVisualization();
}

void searchRec(Node* node, int value, std::vector<int>& path) {
    if (!node) return;
    
    path.push_back(node->id);
    
    if (node->data == value) return;
    
    if (value < node->data) {
        searchRec(node->left, value, path);
    } else {
        searchRec(node->right, value, path);
    }
}

extern "C" void searchBST(int value) {
    std::vector<int> path;
    searchRec(bstRoot, value, path);
    
    val js_path = val::array();
    for (int id : path) {
        js_path.call<void>("push", id);
    }
    
    val::global("highlightPath").call<void>("call", val::undefined(), js_path);
}

extern "C" void clearBST() {
    bstRoot = nullptr; 
    nextId = 0;
    updateBSTVisualization();
}

EMSCRIPTEN_BINDINGS(tree_module) {
    function("insertBST", &insertBST);
    function("deleteBST", &deleteBST);
    function("searchBST", &searchBST);
    function("clearBST", &clearBST);
    function("setAVL", &setAVL);
}
