#include "osm_parser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Calculate distance between two lat/lon points in meters
double OSMParser::haversineDistance(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371000.0; // Earth radius in meters
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    
    double a = sin(dLat/2) * sin(dLat/2) +
               cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
               sin(dLon/2) * sin(dLon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    
    return R * c;
}

bool OSMParser::parseOSM(const std::string& filename, Graph& graph) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return false;
    }
    
    std::cout << "Parsing OSM file: " << filename << std::endl;
    
    std::string line;
    bool inWay = false;
    bool isHighway = false;
    std::string highwayType = "unclassified";
    std::vector<long long> wayNodes;
    
    int nodeCount = 0;
    int wayCount = 0;
    
    while (std::getline(file, line)) {
        // Parse nodes
        if (line.find("<node") != std::string::npos) {
            long long id;
            double lat, lon;
            
            size_t id_pos = line.find("id=\"");
            size_t lat_pos = line.find("lat=\"");
            size_t lon_pos = line.find("lon=\"");
            
            if (id_pos != std::string::npos && lat_pos != std::string::npos && lon_pos != std::string::npos) {
                sscanf(line.c_str() + id_pos, "id=\"%lld\"", &id);
                sscanf(line.c_str() + lat_pos, "lat=\"%lf\"", &lat);
                sscanf(line.c_str() + lon_pos, "lon=\"%lf\"", &lon);
                
                graph.addNode(id, lat, lon);
                nodeCount++;
                
                if (nodeCount % 10000 == 0) {
                    std::cout << "  Parsed " << nodeCount << " nodes...\r" << std::flush;
                }
            }
        }
        
        // Parse ways (roads)
        if (line.find("<way") != std::string::npos) {
            inWay = true;
            isHighway = false;
            highwayType = "unclassified";
            wayNodes.clear();
        }
        
        if (inWay && line.find("<tag k=\"highway\"") != std::string::npos) {
            isHighway = true;
            // Extract highway type
            size_t v_pos = line.find("v=\"");
            if (v_pos != std::string::npos) {
                size_t v_end = line.find("\"", v_pos + 3);
                if (v_end != std::string::npos) {
                    highwayType = line.substr(v_pos + 3, v_end - v_pos - 3);
                }
            }
        }
        
        if (inWay && line.find("<nd ref=") != std::string::npos) {
            long long node_id;
            size_t ref_pos = line.find("ref=\"");
            if (ref_pos != std::string::npos) {
                sscanf(line.c_str() + ref_pos, "ref=\"%lld\"", &node_id);
                wayNodes.push_back(node_id);
            }
        }
        
        if (line.find("</way>") != std::string::npos) {
            if (isHighway && wayNodes.size() >= 2) {
                for (size_t i = 0; i < wayNodes.size() - 1; i++) {
                    const Node* node1 = graph.getNode(wayNodes[i]);
                    const Node* node2 = graph.getNode(wayNodes[i + 1]);
                    
                    if (node1 && node2) {
                        double dist = haversineDistance(node1->lat, node1->lon, 
                                                       node2->lat, node2->lon);
                        graph.addEdge(wayNodes[i], wayNodes[i + 1], dist, highwayType);
                        graph.addEdge(wayNodes[i + 1], wayNodes[i], dist, highwayType);
                    }
                }
                wayCount++;
                
                if (wayCount % 1000 == 0) {
                    std::cout << "  Parsed " << wayCount << " ways...\r" << std::flush;
                }
            }
            inWay = false;
        }
    }
    
    file.close();
    std::cout << "\nParsing complete!" << std::endl;
    std::cout << "  Total nodes: " << nodeCount << std::endl;
    std::cout << "  Total ways: " << wayCount << std::endl;
    return true;
}