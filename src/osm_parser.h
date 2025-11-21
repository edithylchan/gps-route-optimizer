#ifndef OSM_PARSER_H
#define OSM_PARSER_H

#include "graph.h"
#include <string>

class OSMParser {
public:
    static bool parseOSM(const std::string& filename, Graph& graph);
    
private:
    static double haversineDistance(double lat1, double lon1, double lat2, double lon2);
};

#endif