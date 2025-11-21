#include "graph.h"
#include <iostream>
#include <queue>
#include <cmath>
#include <algorithm>
#include <random>

void Graph::addNode(long long id, double lat, double lon) {
    nodes[id] = {id, lat, lon};
}

void Graph::addEdge(long long from, long long to, double distance, 
                    const std::string& road_type) {
    // Default speed limits by road type (km/h)
    double speed_limit = 50.0; // default
    
    if (road_type == "motorway" || road_type == "motorway_link") {
        speed_limit = 100.0;
    } else if (road_type == "trunk" || road_type == "trunk_link") {
        speed_limit = 80.0;
    } else if (road_type == "primary" || road_type == "primary_link") {
        speed_limit = 65.0;
    } else if (road_type == "secondary") {
        speed_limit = 55.0;
    } else if (road_type == "tertiary" || road_type == "residential") {
        speed_limit = 40.0;
    } else if (road_type == "living_street") {
        speed_limit = 20.0;
    }
    
    adjacency_list[from].push_back({to, distance, speed_limit, road_type, 1.0});
}

const Node* Graph::getNode(long long id) const {
    auto it = nodes.find(id);
    return (it != nodes.end()) ? &(it->second) : nullptr;
}

const std::vector<Edge>* Graph::getEdges(long long id) const {
    auto it = adjacency_list.find(id);
    return (it != adjacency_list.end()) ? &(it->second) : nullptr;
}

size_t Graph::edgeCount() const {
    size_t count = 0;
    for (const auto& pair : adjacency_list) {
        count += pair.second.size();
    }
    return count;
}

void Graph::printStats() const {
    std::cout << "Graph Statistics:\n";
    std::cout << "  Nodes: " << nodeCount() << "\n";
    std::cout << "  Edges: " << edgeCount() << "\n";
}

// Simulate learned patterns from crowd-sourced data
void Graph::applyLearnedPatterns() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    int shortcuts_found = 0;
    int congestion_points = 0;
    
    for (auto& node_pair : adjacency_list) {
        for (auto& edge : node_pair.second) {
            // Motorways and trunks sometimes have hidden congestion
            if ((edge.road_type == "motorway" || edge.road_type == "trunk") && dis(gen) < 0.05) {
                edge.crowd_multiplier = 0.6;  // 40% slower than expected (congestion)
                congestion_points++;
            }
            
            // Some primary/secondary roads are "local shortcuts" - faster than expected
            if ((edge.road_type == "primary" || edge.road_type == "secondary") && dis(gen) < 0.03) {
                edge.crowd_multiplier = 1.4;  // 40% faster (local knowledge)
                shortcuts_found++;
            }
            
            // Residential streets near motorways might be shortcuts
            if (edge.road_type == "residential" && dis(gen) < 0.02) {
                edge.crowd_multiplier = 1.2;  // 20% faster (parallel route)
                shortcuts_found++;
            }
        }
    }
    
    std::cout << "\nApplied crowd-sourced learning patterns:\n";
    std::cout << "  Hidden shortcuts discovered: " << shortcuts_found << "\n";
    std::cout << "  Congestion points identified: " << congestion_points << "\n";
}

// Calculate time-adjusted speed based on hour of day
double Graph::getTimeAdjustedSpeed(const Edge& edge, int hour_of_day) const {
    double base_speed = edge.speed_limit;
    
    // Morning rush hour (7-9 AM)
    bool morning_rush = (hour_of_day >= 7 && hour_of_day <= 9);
    // Evening rush hour (5-7 PM)
    bool evening_rush = (hour_of_day >= 17 && hour_of_day <= 19);
    
    if (morning_rush || evening_rush) {
        if (edge.road_type == "motorway" || edge.road_type == "trunk") {
            base_speed *= 0.4;  // Highways 60% slower in rush hour
        } else if (edge.road_type == "primary") {
            base_speed *= 0.6;  // Major roads 40% slower
        } else if (edge.road_type == "secondary" || edge.road_type == "tertiary") {
            base_speed *= 0.8;  // Minor roads only 20% slower
        }
        // Residential streets mostly unaffected
    }
    
    return base_speed;
}

// Calculate edge weight based on routing mode
double Graph::calculateEdgeWeight(const Edge& edge, RouteMode mode, int hour_of_day) const {
    switch (mode) {
        case RouteMode::DISTANCE:
            // Pure distance - no speed consideration
            return edge.distance;
            
        case RouteMode::SPEED_LIMIT:
            // Traditional GPS: distance / speed limit (in m/s)
            return edge.distance / (edge.speed_limit * 1000.0 / 3600.0);
            
        case RouteMode::LEARNED: {
            // Advanced: time-aware + crowd-sourced data
            double adjusted_speed = getTimeAdjustedSpeed(edge, hour_of_day);
            adjusted_speed *= edge.crowd_multiplier;  // Apply learned patterns
            return edge.distance / (adjusted_speed * 1000.0 / 3600.0);
        }
    }
    return edge.distance;
}

// Enhanced Dijkstra with routing modes
RouteResult Graph::dijkstra(long long start_id, long long end_id, 
                            RouteMode mode, int hour_of_day) const {
    RouteResult result;
    result.mode = mode;
    result.total_distance = 0.0;
    result.estimated_time = 0.0;
    
    // Set mode name
    switch (mode) {
        case RouteMode::DISTANCE:
            result.mode_name = "Pure Distance";
            break;
        case RouteMode::SPEED_LIMIT:
            result.mode_name = "Speed Limit (Traditional GPS)";
            break;
        case RouteMode::LEARNED:
            result.mode_name = "Learned Patterns (Advanced)";
            break;
    }
    
    std::unordered_map<long long, double> distances;
    std::unordered_map<long long, long long> previous;
    
    for (const auto& node_pair : nodes) {
        distances[node_pair.first] = std::numeric_limits<double>::infinity();
    }
    distances[start_id] = 0.0;
    
    using PQElement = std::pair<double, long long>;
    std::priority_queue<PQElement, std::vector<PQElement>, std::greater<PQElement>> pq;
    pq.push({0.0, start_id});
    
    while (!pq.empty()) {
        auto [current_dist, current_id] = pq.top();
        pq.pop();
        
        if (current_id == end_id) {
            break;
        }
        
        if (current_dist > distances[current_id]) {
            continue;
        }
        
        const auto* edges = getEdges(current_id);
        if (edges) {
            for (const auto& edge : *edges) {
                double weight = calculateEdgeWeight(edge, mode, hour_of_day);
                double new_dist = current_dist + weight;
                
                if (new_dist < distances[edge.to]) {
                    distances[edge.to] = new_dist;
                    previous[edge.to] = current_id;
                    pq.push({new_dist, edge.to});
                }
            }
        }
    }
    
    // Reconstruct path
    if (distances[end_id] == std::numeric_limits<double>::infinity()) {
        return result;  // No path found
    }
    
    long long current = end_id;
    while (current != start_id) {
        result.path.push_back(current);
        current = previous[current];
    }
    result.path.push_back(start_id);
    std::reverse(result.path.begin(), result.path.end());
    
    // Calculate actual distance and time
    for (size_t i = 0; i < result.path.size() - 1; i++) {
        const auto* edges = getEdges(result.path[i]);
        if (edges) {
            for (const auto& edge : *edges) {
                if (edge.to == result.path[i + 1]) {
                    result.total_distance += edge.distance;
                    
                    // Calculate time based on learned speed
                    double speed = getTimeAdjustedSpeed(edge, hour_of_day) * edge.crowd_multiplier;
                    result.estimated_time += edge.distance / (speed * 1000.0 / 3600.0);
                    break;
                }
            }
        }
    }
    
    return result;
}