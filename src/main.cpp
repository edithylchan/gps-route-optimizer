#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <random>
#include <algorithm>
#include "graph.h"
#include "osm_parser.h"

void exportRouteToJSON(const Graph& graph, const std::vector<RouteResult>& routes, 
                       const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not create JSON file\n";
        return;
    }
    
    file << "{\n";
    file << "  \"routes\": [\n";
    
    for (size_t r = 0; r < routes.size(); r++) {
        const auto& route = routes[r];
        
        file << "    {\n";
        file << "      \"mode\": \"" << route.mode_name << "\",\n";
        file << "      \"color\": \"";
        
        // Assign colors
        if (route.mode == RouteMode::DISTANCE) {
            file << "#FF6B6B";  // Red
        } else if (route.mode == RouteMode::SPEED_LIMIT) {
            file << "#4ECDC4";  // Cyan
        } else {
            file << "#95E1D3";  // Green
        }
        file << "\",\n";
        
        file << "      \"total_distance_km\": " << std::fixed << std::setprecision(3) 
             << (route.total_distance / 1000.0) << ",\n";
        file << "      \"estimated_time_min\": " << std::fixed << std::setprecision(1) 
             << (route.estimated_time / 60.0) << ",\n";
        file << "      \"waypoints\": [\n";
        
        for (size_t i = 0; i < route.path.size(); i++) {
            const Node* node = graph.getNode(route.path[i]);
            file << "        {\n";
            file << "          \"id\": " << route.path[i] << ",\n";
            file << "          \"lat\": " << std::fixed << std::setprecision(7) << node->lat << ",\n";
            file << "          \"lon\": " << node->lon << "\n";
            file << "        }";
            if (i < route.path.size() - 1) file << ",";
            file << "\n";
        }
        
        file << "      ]\n";
        file << "    }";
        if (r < routes.size() - 1) file << ",";
        file << "\n";
    }
    
    file << "  ]\n";
    file << "}\n";
    
    file.close();
    std::cout << "\nRoutes exported to " << filename << "\n";
}

void printRouteComparison(const std::vector<RouteResult>& routes) {
    std::cout << "\n================================================================\n";
    std::cout << "           ROUTE COMPARISON: 3 OPTIMIZATION METHODS            \n";
    std::cout << "================================================================\n\n";
    
    // Find baseline (speed limit route)
    const RouteResult* baseline = nullptr;
    for (const auto& route : routes) {
        if (route.mode == RouteMode::SPEED_LIMIT) {
            baseline = &route;
            break;
        }
    }
    
    for (const auto& route : routes) {
        std::cout << "+--- " << route.mode_name << " ";
        for (int i = 0; i < 50 - route.mode_name.length(); i++) std::cout << "-";
        std::cout << "+\n";
        
        std::cout << "| Distance:       " << std::fixed << std::setprecision(2) 
                  << (route.total_distance / 1000.0) << " km\n";
        std::cout << "| Estimated Time: " << std::fixed << std::setprecision(1) 
                  << (route.estimated_time / 60.0) << " minutes\n";
        std::cout << "| Waypoints:      " << route.path.size() << " nodes\n";
        
        if (baseline && route.mode != RouteMode::SPEED_LIMIT) {
            double time_diff = route.estimated_time - baseline->estimated_time;
            double dist_diff = route.total_distance - baseline->total_distance;
            
            std::cout << "| vs Traditional: ";
            if (time_diff < 0) {
                std::cout << ">> " << std::fixed << std::setprecision(1) 
                         << (-time_diff / 60.0) << " min FASTER";
            } else {
                std::cout << "<< " << std::fixed << std::setprecision(1) 
                         << (time_diff / 60.0) << " min slower";
            }
            
            std::cout << " (";
            if (dist_diff > 0) {
                std::cout << "+" << std::fixed << std::setprecision(2) 
                         << (dist_diff / 1000.0) << " km longer";
            } else {
                std::cout << std::fixed << std::setprecision(2) 
                         << (dist_diff / 1000.0) << " km shorter";
            }
            std::cout << ")\n";
        }
        
        std::cout << "+";
        for (int i = 0; i < 63; i++) std::cout << "-";
        std::cout << "+\n\n";
    }
    
    // Show insights
    std::cout << "*** KEY INSIGHTS:\n";
    
    const RouteResult* distance_route = nullptr;
    const RouteResult* learned_route = nullptr;
    
    for (const auto& route : routes) {
        if (route.mode == RouteMode::DISTANCE) distance_route = &route;
        if (route.mode == RouteMode::LEARNED) learned_route = &route;
    }
    
    if (distance_route && baseline) {
        std::cout << "   * Shortest distance != fastest time!\n";
        std::cout << "     Distance route is " << std::fixed << std::setprecision(1)
                  << ((distance_route->estimated_time - baseline->estimated_time) / 60.0)
                  << " min slower despite being shorter.\n";
    }
    
    if (learned_route && baseline) {
        double time_saved = (baseline->estimated_time - learned_route->estimated_time) / 60.0;
        if (time_saved > 0) {
            std::cout << "   * Crowd-sourced learning saves " << std::fixed << std::setprecision(1)
                      << time_saved << " minutes!\n";
            std::cout << "     The learned route finds shortcuts that traditional GPS misses.\n";
        }
    }
    
    std::cout << "\n";
}

std::vector<long long> getRandomConnectedNodes(const Graph& graph, int count = 10) {
    std::vector<long long> candidates;
    
    int samples = 0;
    const int maxSamples = 5000;
    
    for (long long id = 1; id < 10000000 && samples < maxSamples; id++) {
        const auto* edges = graph.getEdges(id);
        if (edges && !edges->empty()) {
            candidates.push_back(id);
            samples++;
        }
    }
    
    if (candidates.size() < 2) {
        return {};
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(candidates.begin(), candidates.end(), gen);
    
    int returnCount = std::min(count, (int)candidates.size());
    return std::vector<long long>(candidates.begin(), candidates.begin() + returnCount);
}

int main() {
    std::cout << "\n";
    std::cout << "================================================================\n";
    std::cout << "       GPS ROUTE OPTIMIZER: Evidence-Based Routing Demo       \n";
    std::cout << "                                                               \n";
    std::cout << "  Comparing Traditional GPS vs. Learned Traffic Patterns      \n";
    std::cout << "================================================================\n\n";
    
    Graph graph;
    
    std::cout << "Loading OpenStreetMap data...\n";
    if (!OSMParser::parseOSM("data/map.osm", graph)) {
        std::cerr << "Failed to parse OSM file" << std::endl;
        return 1;
    }
    
    std::cout << "\n";
    graph.printStats();
    
    std::cout << "\nApplying crowd-sourced learning patterns...\n";
    std::cout << "   (Simulating data from millions of real drives)\n";
    graph.applyLearnedPatterns();
    
    std::cout << "\nFinding sample routes...\n";
    auto sampleNodes = getRandomConnectedNodes(graph, 10);
    
    if (sampleNodes.size() < 2) {
        std::cout << "Could not find connected nodes in the graph.\n";
        return 1;
    }
    
    // Calculate routes using all 3 methods
    std::cout << "\nCalculating routes using 3 different optimization strategies...\n";
    std::cout << "   Start: Node " << sampleNodes[0] << "\n";
    std::cout << "   End:   Node " << sampleNodes[1] << "\n";
    
    // Use evening rush hour (5 PM) to show maximum difference
    int hour = 17;
    
    std::vector<RouteResult> routes;
    
    std::cout << "\n   [1/3] Pure distance optimization...\n";
    routes.push_back(graph.dijkstra(sampleNodes[0], sampleNodes[1], RouteMode::DISTANCE, hour));
    
    std::cout << "   [2/3] Speed limit optimization (Traditional GPS)...\n";
    routes.push_back(graph.dijkstra(sampleNodes[0], sampleNodes[1], RouteMode::SPEED_LIMIT, hour));
    
    std::cout << "   [3/3] Learned pattern optimization (Advanced)...\n";
    routes.push_back(graph.dijkstra(sampleNodes[0], sampleNodes[1], RouteMode::LEARNED, hour));
    
    // Print comparison
    printRouteComparison(routes);
    
    // Export for visualization
    exportRouteToJSON(graph, routes, "web/routes.json");

    std::cout << "Open web/index.html in your browser to see the routes visualized!\n\n";

    // Interactive mode
    std::cout << "================================================================\n";
    std::cout << "INTERACTIVE MODE\n";
    std::cout << "================================================================\n\n";

    std::cout << "Sample node IDs you can try:\n";
    for (size_t i = 0; i < std::min((size_t)5, sampleNodes.size()); i++) {
        std::cout << "  " << (i + 1) << ". Node " << sampleNodes[i] << "\n";
    }
    
    while (true) {
        std::cout << "\nEnter start node ID (or 0 to quit): ";
        long long start;
        std::cin >> start;
        
        if (start == 0) break;
        
        std::cout << "Enter end node ID: ";
        long long end;
        std::cin >> end;
        
        std::cout << "Enter hour of day (0-23, or 12 for noon): ";
        int user_hour;
        std::cin >> user_hour;
        
        if (!graph.getNode(start) || !graph.getNode(end)) {
            std::cout << "Invalid node IDs!\n";
            continue;
        }
        
        std::cout << "\nCalculating routes...\n";
        
        std::vector<RouteResult> custom_routes;
        custom_routes.push_back(graph.dijkstra(start, end, RouteMode::DISTANCE, user_hour));
        custom_routes.push_back(graph.dijkstra(start, end, RouteMode::SPEED_LIMIT, user_hour));
        custom_routes.push_back(graph.dijkstra(start, end, RouteMode::LEARNED, user_hour));
        
        printRouteComparison(custom_routes);
        exportRouteToJSON(graph, custom_routes, "web/routes.json");
        
        std::cout << "Routes updated in web visualization!\n";
    }
    
    std::cout << "\nThanks for exploring Evidence-Based Routing!\n\n";
    return 0;
}