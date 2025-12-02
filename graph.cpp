#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <stack>
#include <map>
#include <limits>
#include <algorithm>
#include <set>

using namespace emscripten;

struct Edge {
    int target;
    int weight;
};

// Adjacency List: node_id -> list of edges
std::map<int, std::vector<Edge>> adj;
std::set<int> nodes;

// Helper to log events to JS
void logEvent(std::string type, val data, std::string message) {
    val::global("handleEvent").call<void>("call", val::undefined(), type, data, message);
}

// Helper to get graph data for visualization
val getGraphData() {
    val nodesArray = val::array();
    for (int id : nodes) {
        val nodeObj = val::object();
        nodeObj.set("id", id);
        nodesArray.call<void>("push", nodeObj);
    }

    val linksArray = val::array();
    for (auto const& [u, edges] : adj) {
        for (const auto& edge : edges) {
            val linkObj = val::object();
            linkObj.set("source", u);
            linkObj.set("target", edge.target);
            linkObj.set("weight", edge.weight);
            linksArray.call<void>("push", linkObj);
        }
    }

    val graphData = val::object();
    graphData.set("nodes", nodesArray);
    graphData.set("links", linksArray);
    return graphData;
}

void updateGraphVisualization(std::string message) {
    logEvent("snapshot", getGraphData(), message);
}

extern "C" {

void addNode(int id) {
    if (nodes.find(id) == nodes.end()) {
        nodes.insert(id);
        updateGraphVisualization("Added Node " + std::to_string(id));
    }
}

void addEdge(int source, int target, int weight) {
    addNode(source);
    addNode(target);
    
    // Check if edge exists to update weight or avoid duplicates (assuming directed for now, or undirected?)
    // The prompt says "Add/remove nodes and edges". Let's assume directed for generality, or undirected?
    // Most visualizers do directed or undirected. Let's do Directed for now, or maybe Undirected is better for MST?
    // MST (Prim/Kruskal) usually on Undirected. Dijkstra on Directed/Undirected.
    // Let's make it Undirected for MST compatibility, or support both?
    // Let's stick to Undirected for simplicity with MST.
    
    // Remove existing if any
    auto& edgesSource = adj[source];
    edgesSource.erase(std::remove_if(edgesSource.begin(), edgesSource.end(), 
        [target](const Edge& e){ return e.target == target; }), edgesSource.end());
        
    auto& edgesTarget = adj[target];
    edgesTarget.erase(std::remove_if(edgesTarget.begin(), edgesTarget.end(), 
        [source](const Edge& e){ return e.target == source; }), edgesTarget.end());

    adj[source].push_back({target, weight});
    adj[target].push_back({source, weight}); // Undirected
    
    updateGraphVisualization("Added Edge " + std::to_string(source) + "-" + std::to_string(target));
}

void removeNode(int id) {
    if (nodes.erase(id)) {
        adj.erase(id);
        // Remove edges pointing to this node
        for (auto& [u, edges] : adj) {
            edges.erase(std::remove_if(edges.begin(), edges.end(), 
                [id](const Edge& e){ return e.target == id; }), edges.end());
        }
        updateGraphVisualization("Removed Node " + std::to_string(id));
    }
}

void removeEdge(int source, int target) {
    bool changed = false;
    auto& edgesSource = adj[source];
    auto it1 = std::remove_if(edgesSource.begin(), edgesSource.end(), 
        [target](const Edge& e){ return e.target == target; });
    if (it1 != edgesSource.end()) {
        edgesSource.erase(it1, edgesSource.end());
        changed = true;
    }

    auto& edgesTarget = adj[target];
    auto it2 = std::remove_if(edgesTarget.begin(), edgesTarget.end(), 
        [source](const Edge& e){ return e.target == source; });
    if (it2 != edgesTarget.end()) {
        edgesTarget.erase(it2, edgesTarget.end());
        changed = true;
    }
    
    if (changed) updateGraphVisualization("Removed Edge " + std::to_string(source) + "-" + std::to_string(target));
}

void bfs(int startNode) {
    if (nodes.find(startNode) == nodes.end()) return;

    std::queue<int> q;
    std::set<int> visited;
    std::vector<int> traversalOrder;

    q.push(startNode);
    visited.insert(startNode);

    while (!q.empty()) {
        int u = q.front();
        q.pop();
        traversalOrder.push_back(u);

        // Highlight current node
        val highlightData = val::object();
        highlightData.set("node", u);
        val qArray = val::array();
        // Queue copy for visualization (inefficient but simple)
        std::queue<int> tempQ = q;
        while(!tempQ.empty()) {
            qArray.call<void>("push", tempQ.front());
            tempQ.pop();
        }
        highlightData.set("queue", qArray);
        logEvent("highlight", highlightData, "Visiting " + std::to_string(u));

        // Sort neighbors for consistent traversal if needed, but vector order is fine
        for (const auto& edge : adj[u]) {
            if (visited.find(edge.target) == visited.end()) {
                visited.insert(edge.target);
                q.push(edge.target);
            }
        }
    }
    logEvent("finished", val::null(), "BFS Completed");
}

void dfs(int startNode) {
    if (nodes.find(startNode) == nodes.end()) return;

    std::stack<int> s;
    std::set<int> visited;
    std::vector<int> traversalOrder;

    s.push(startNode);

    while (!s.empty()) {
        int u = s.top();
        s.pop();

        if (visited.find(u) != visited.end()) continue;
        visited.insert(u);
        traversalOrder.push_back(u);

        // Highlight
        val highlightData = val::object();
        highlightData.set("node", u);
        val sArray = val::array();
        std::stack<int> tempS = s;
        while(!tempS.empty()) {
            sArray.call<void>("push", tempS.top());
            tempS.pop();
        }
        highlightData.set("stack", sArray);
        logEvent("highlight", highlightData, "Visiting " + std::to_string(u));

        // Push neighbors (reverse order to visit in increasing order if sorted, but here just push)
        // For standard DFS, we usually push all neighbors.
        for (const auto& edge : adj[u]) {
            if (visited.find(edge.target) == visited.end()) {
                s.push(edge.target);
            }
        }
    }
    logEvent("finished", val::null(), "DFS Completed");
}

void prim(int startNode) {
    if (nodes.find(startNode) == nodes.end()) return;

    // Priority Queue: <weight, target_node, source_node>
    // We need source_node to identify the edge for visualization
    using PII = std::tuple<int, int, int>; 
    std::priority_queue<PII, std::vector<PII>, std::greater<PII>> pq;

    std::set<int> visited;
    std::vector<std::pair<int, int>> mstEdges;

    pq.push({0, startNode, -1});

    while (!pq.empty()) {
        auto [w, u, parent] = pq.top();
        pq.pop();

        if (visited.find(u) != visited.end()) continue;
        visited.insert(u);

        if (parent != -1) {
            mstEdges.push_back({parent, u});
            // Highlight MST edge
            val edgeData = val::object();
            edgeData.set("source", parent);
            edgeData.set("target", u);
            logEvent("mst_edge", edgeData, "Added to MST: " + std::to_string(parent) + "-" + std::to_string(u));
        }

        for (const auto& edge : adj[u]) {
            if (visited.find(edge.target) == visited.end()) {
                pq.push({edge.weight, edge.target, u});
            }
        }
    }
    logEvent("finished", val::null(), "Prim's Algorithm Completed");
}

void dijkstra(int startNode, int endNode) {
    if (nodes.find(startNode) == nodes.end()) return;

    std::map<int, int> dist;
    std::map<int, int> parent;
    for (int id : nodes) dist[id] = std::numeric_limits<int>::max();

    dist[startNode] = 0;
    
    // <distance, node>
    std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::greater<std::pair<int, int>>> pq;
    pq.push({0, startNode});

    while (!pq.empty()) {
        int d = pq.top().first;
        int u = pq.top().second;
        pq.pop();

        if (d > dist[u]) continue;

        // Visual update
        val visitData = val::object();
        visitData.set("node", u);
        visitData.set("dist", d);
        logEvent("visit_node", visitData, "Relaxing Node " + std::to_string(u));

        if (u == endNode) break;

        for (const auto& edge : adj[u]) {
            if (dist[u] + edge.weight < dist[edge.target]) {
                dist[edge.target] = dist[u] + edge.weight;
                parent[edge.target] = u;
                pq.push({dist[edge.target], edge.target});
                
                // Visual update for relaxation
                val relaxData = val::object();
                relaxData.set("source", u);
                relaxData.set("target", edge.target);
                relaxData.set("newDist", dist[edge.target]);
                logEvent("relax_edge", relaxData, "Updated distance to " + std::to_string(edge.target));
            }
        }
    }
    
    // Reconstruct path
    if (dist[endNode] != std::numeric_limits<int>::max()) {
        std::vector<int> path;
        int curr = endNode;
        while (curr != startNode) {
            path.push_back(curr);
            curr = parent[curr];
        }
        path.push_back(startNode);
        std::reverse(path.begin(), path.end());
        
        val pathArray = val::array();
        for (int id : path) pathArray.call<void>("push", id);
        logEvent("shortest_path", pathArray, "Shortest Path Found");
    } else {
        logEvent("finished", val::null(), "No path found");
    }
}

void clearGraph() {
    adj.clear();
    nodes.clear();
    updateGraphVisualization("Graph Cleared");
}

} // extern "C"

EMSCRIPTEN_BINDINGS(graph_module) {
    function("addNode", &addNode);
    function("addEdge", &addEdge);
    function("removeNode", &removeNode);
    function("removeEdge", &removeEdge);
    function("bfs", &bfs);
    function("dfs", &dfs);
    function("prim", &prim);
    function("dijkstra", &dijkstra);
    function("clearGraph", &clearGraph);
}
