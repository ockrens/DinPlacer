#ifndef DXFHANDLING_HPP
#define DXFHANDLING_HPP

#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <algorithm>
#include "dxflib/dl_dxf.h"
#include "dxflib/dl_creationadapter.h"
// #include "main.hpp"

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
        int instanceID; // Dit moet hier staan
    };

    std::string currentBlockName = "";
    std::map<std::string, std::vector<LocalVertex>> blockLibrary;
    std::vector<TempPoint> tempPoints;
    size_t lastInsertStartIndex = 0;
    int globalInstanceCounter = 0; // Teller toevoegen

public:
    std::vector<BlockResult> finalBlocks;
    std::map<int, std::vector<BlockResult>> rails;

    void addBlock(const DL_BlockData& data) override;
    void endBlock() override;
    void addVertex(const DL_VertexData& data) override;
    void addLine(const DL_LineData& data) override;
    void addInsert(const DL_InsertData& data) override;
    void addAttribute(const DL_AttributeData& data) override;

    bool loadFile(const std::string& filename);
    void processBlocks(double maxGrens); 
    void groupAndSortByRails(double yThreshold);
    void printFinalResults() const;
};

#endif
