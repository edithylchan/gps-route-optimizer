#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include <unordered_map>
#include <vector>
#include <limits>

struct Node {
    long long id;
    double lat;
    double lon;
};

struct Edge {
    long long to;
    double distance;           // meters
    double speed_limit;        // km/h
    std::string road_type;     // motorway, primary, residential, etc.
    double crowd_multiplier;   // learned speed adjustment (1.0 = normal, 1.3 = 30% faster)
};

enum class RouteMode {
    DISTANCE,      // Pure shortest distance
    SPEED_LIMIT,   // Speed limit-based (traditional GPS)
    LEARNED        // Crowd-sourced learned patterns
};

struct RouteResult {
    std::vector<long long> path;
    double total_distance;     // meters
    double estimated_time;     // seconds
    RouteMode mode;
    std::string mode_name;
};

class Graph {
private:
    std::unordered_map<long long, Node> nodes;
    std::unordered_map<long long, std::vector<Edge>> adjacency_list;
    
    double calculateEdgeWeight(const Edge& edge, RouteMode mode, int hour_of_day) const;
    double getTimeAdjustedSpeed(const Edge& edge, int hour_of_day) const;

public:
    void addNode(long long id, double lat, double lon);
    void addEdge(long long from, long long to, double distance, 
                 const std::string& road_type = "unclassified");
    
    const Node* getNode(long long id) const;
    const std::vector<Edge>* getEdges(long long id) const;
    
    // Enhanced routing with different modes
    RouteResult dijkstra(long long start_id, long long end_id, 
                        RouteMode mode = RouteMode::SPEED_LIMIT,
                        int hour_of_day = 12) const;
    
    // Simulate crowd-sourced learning on certain edges
    void applyLearnedPatterns();
    
    size_t nodeCount() const { return nodes.size(); }
    size_t edgeCount() const;
    
    void printStats() const;
};

#endif