#include "../include/DXFHandling.hpp"
#include <algorithm>
#include <iostream>
#include <cmath> // Voor std::abs

// --- STAP 1: BIBLIOTHEEK VULLEN (Blauwdrukken) ---

void DXFManager::addBlock(const DL_BlockData& data) {
    currentBlockName = data.name;
}

void DXFManager::endBlock() {
    currentBlockName = "";
}

void DXFManager::addVertex(const DL_VertexData& data) {
    if (!currentBlockName.empty()) {
        blockLibrary[currentBlockName].push_back({data.x, data.y});
    }
}

void DXFManager::addLine(const DL_LineData& data) {
    if (!currentBlockName.empty()) {
        blockLibrary[currentBlockName].push_back({data.x1, data.y1});
        blockLibrary[currentBlockName].push_back({data.x2, data.y2});
    }
}

// --- STAP 2: INSTANTIES PLAATSEN (De tekening scannen) ---

void DXFManager::addInsert(const DL_InsertData& data) {
    if (blockLibrary.count(data.name)) {
        lastInsertStartIndex = tempPoints.size();
        for (const auto& local : blockLibrary[data.name]) {
            double wx = (local.x * data.sx) + data.ipx;
            double wy = (local.y * data.sy) + data.ipy;
            tempPoints.push_back({"ONBEKEND", data.name, wx, wy, globalInstanceCounter});
        }
        globalInstanceCounter++;
    }
}

void DXFManager::addAttribute(const DL_AttributeData& data) {
    for (size_t i = lastInsertStartIndex; i < tempPoints.size(); ++i) {
        tempPoints[i].label = data.text;
    }
}

// --- STAP 3: DATA VERWERKEN (Spiegelen & Filteren) ---

bool DXFManager::loadFile(const std::string& filename) {
    DL_Dxf dxf;
    return dxf.in(filename, this);
}

void DXFManager::processBlocks(double maxGrens) {
    if (tempPoints.empty()) return;

    // We bepalen eerst de uitersten van de ruwe data
    double gMinX = tempPoints[0].x; double gMaxX = tempPoints[0].x;
    double gMinY = tempPoints[0].y; double gMaxY = tempPoints[0].y;
    for (const auto& p : tempPoints) {
        gMinX = std::min(gMinX, p.x); gMaxX = std::max(gMaxX, p.x);
        gMinY = std::min(gMinY, p.y); gMaxY = std::max(gMaxY, p.y);
    }

    finalBlocks.clear();
    size_t i = 0;
    while (i < tempPoints.size()) {
        int currentID = tempPoints[i].instanceID;
        double bMinX = 1e15, bMaxX = -1e15, bMinY = 1e15, bMaxY = -1e15;

        size_t j = i;
        while (j < tempPoints.size() && tempPoints[j].instanceID == currentID) {
            // --- X EN Y COÖRDINATEN TERUGGEDRAAID ---
            // DXF X-data stuurt nu weer jouw X-as aan (min gMinX om bij 0 te beginnen)
            // DXF Y-data stuurt nu weer jouw Y-as aan (gMaxY - p.y voor de verticale spiegeling)
            double correctedX = tempPoints[j].x - gMinX;
            double correctedY = gMaxY - tempPoints[j].y;

            bMinX = std::min(bMinX, correctedX); bMaxX = std::max(bMaxX, correctedX);
            bMinY = std::min(bMinY, correctedY); bMaxY = std::max(bMaxY, correctedY);
            j++;
        }

        // --- STAP 3B: FILTERING OP GROOTTE ---
        double breedte = bMaxX - bMinX;
        double hoogte = bMaxY - bMinY;

        if (breedte > maxGrens || hoogte > maxGrens) {
            i = j;
            continue;
        }
        BlockResult res;
        res.label = (tempPoints[i].label == "ONBEKEND") ? (tempPoints[i].type + "_" + std::to_string(currentID)) : tempPoints[i].label;
        res.type = tempPoints[i].type;
        res.minX = bMinX; res.minY = bMinY; res.maxX = bMaxX; res.maxY = bMaxY;
        res.centerX = (bMinX + bMaxX) / 2.0;
        res.centerY = (bMinY + bMaxY) / 2.0;
        
        finalBlocks.push_back(res);
        i = j;
    }
    tempPoints.clear();
}

// --- STAP 4: RAILS SORTEREN (Groeperen en van Links naar Rechts) ---

void DXFManager::groupAndSortByRails(double yThreshold) {
    rails.clear();
    if (finalBlocks.empty()) return;

    std::vector<BlockResult> sortedY = finalBlocks;
    std::sort(sortedY.begin(), sortedY.end(), [](const BlockResult& a, const BlockResult& b) {
        return a.centerY < b.centerY; 
    });

    int railIndex = 0;
    if (!sortedY.empty()) {
        double currentRailY = sortedY[0].centerY;

        for (const auto& b : sortedY) {
            if (std::abs(b.centerY - currentRailY) > yThreshold) {
                railIndex++;
                currentRailY = b.centerY;
            }
            rails[railIndex].push_back(b);
        }
    }

    for (auto& [id, componenten] : rails) {
        std::sort(componenten.begin(), componenten.end(), [](const BlockResult& a, const BlockResult& b) {
            return a.centerX < b.centerX;
        });
    }
}

void DXFManager::printFinalResults() const {
    std::cout << "\n--- GEVONDEN RAILS ---" << std::endl;
    for (auto const& [id, lijst] : rails) {
        std::cout << "RAIL " << id << " (" << lijst.size() << " componenten):" << std::endl;
        for (const auto& c : lijst) {
            std::cout << "  -> " << c.type << " op X: " << (int)c.centerX << " Y: " << (int)c.centerY << std::endl;
        }
    }
}
