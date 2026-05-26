#ifndef DXFHANDLING_HPP
#define DXFHANDLING_HPP

#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <algorithm>
#include "dxflib/dl_dxf.h"
#include "dxflib/dl_creationadapter.h"

struct LocalVertex { double x, y; };

struct BlockResult {
    std::string label;
    std::string type;
    double centerX, centerY;
    double minX, minY;
    double maxX, maxY;
};

class DXFManager : public DL_CreationAdapter {
private:
    struct TempPoint { 
        std::string label; 
        std::string type; 
        double x, y; 
        int instanceID; 
    };

    std::string currentBlockName = "";
    std::map<std::string, std::vector<LocalVertex>> blockLibrary;
    std::vector<TempPoint> tempPoints;
    size_t lastInsertStartIndex = 0;
    int globalInstanceCounter = 0; 
    
    // Slaat de ruwe $INSUNITS waarde op uit de DXF header
    int dxfUnits = 0; 



public:
    std::vector<BlockResult> finalBlocks;
    std::map<int, std::vector<BlockResult>> rails;

    // --- HOOFD OVERRIDES (Geregeld door dxflib) ---
    // Juiste dxflib-functie om header variabelen op te vangen:
    void setVariableInt(const std::string& key, int value, int code) override; 
    
    void addBlock(const DL_BlockData& data) override;
    void endBlock() override;
    void addVertex(const DL_VertexData& data) override;
    void addLine(const DL_LineData& data) override;
    void addInsert(const DL_InsertData& data) override;
    void addAttribute(const DL_AttributeData& data) override;

    // --- VERWERKINGS FUNCTIES ---
    bool loadFile(const std::string& filename);
    void processBlocks(double maxGrens); 
    void groupAndSortByRails(double yThreshold);
    void printFinalResults() const;
    
    // --- EXTRA UTILITIES ---
    std::string getUnitsString() const; 
    int getRawUnits() const { return dxfUnits; }

    double panelWidthMeters = 0.0;
    double panelHeightMeters = 0.0;
};

#endif
