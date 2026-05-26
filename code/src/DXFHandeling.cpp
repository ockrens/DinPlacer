#include "../include/DXFHandling.hpp"
#include <algorithm>
#include <iostream>
#include <cmath> // Voor std::abs

// --- STAP 0: HEADER / MAATVOERING UITLEZEN ---

void DXFManager::setVariableInt(const std::string& key, int value, int code) {
    if (key == "$INSUNITS") {
        dxfUnits = value;
    }
}

std::string DXFManager::getUnitsString() const {
    switch (dxfUnits) {
        case 1:  return "Inches";
        case 4:  return "Millimeters (mm)";
        case 5:  return "Centimeters (cm)";
        case 6:  return "Meters (m)";
        default: return "Onbekend / Niet gespecificeerd (Default cm)";
    }
}

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

// --- STAP 3: DATA VERWERKEN (Naar Meters Converteren, Spiegelen & Filteren) ---

bool DXFManager::loadFile(const std::string& filename) {
    DL_Dxf dxf;
    dxfUnits = 0; 
    return dxf.in(filename, this);
}

void DXFManager::processBlocks(double maxGrens) {
    if (tempPoints.empty()) return;

    // 1. Conversiefactor naar METERS bepalen (Standaard cm fallback)
    double toMetersFactor = 0.01; 
    if (dxfUnits == 4)      toMetersFactor = 0.001;  // mm naar m
    else if (dxfUnits == 5) toMetersFactor = 0.01;   // cm naar m
    else if (dxfUnits == 6) toMetersFactor = 1.0;    // m naar m
    else if (dxfUnits == 1) toMetersFactor = 0.0254; // inch naar m

    double maxGrensInMeters = maxGrens * toMetersFactor;

    // STAP 1: BEPAAL DE ABSOLUTE GEOMETRISCHE GRENZEN VAN HET DOCUMENT
    double gMinX = tempPoints[0].x; double gMaxX = tempPoints[0].x;
    double gMinY = tempPoints[0].y; double gMaxY = tempPoints[0].y;
    for (const auto& p : tempPoints) {
        gMinX = std::min(gMinX, p.x); gMaxX = std::max(gMaxX, p.x);
        gMinY = std::min(gMinY, p.y); gMaxY = std::max(gMaxY, p.y);
    }

    // Hulplogica om componenten tijdelijk in meters op te slaan
    struct RawComp {
        std::string label; std::string type; int id;
        double rMinX, rMaxX, rMinY, rMaxY;
    };
    std::vector<RawComp> allComponents;

    size_t i = 0;
    while (i < tempPoints.size()) {
        int currentID = tempPoints[i].instanceID;
        double rMinX = 1e15, rMaxX = -1e15, rMinY = 1e15, rMaxY = -1e15;

        size_t j = i;
        while (j < tempPoints.size() && tempPoints[j].instanceID == currentID) {
            // --- GECORRIGEERD: GEEN VERTICALE SPIEGELING MEER ---
            // We behouden de natuurlijke CAD Y-as richting (omhoog = positief)
            double posX = (tempPoints[j].x - gMinX) * toMetersFactor;
            double posY = (tempPoints[j].y - gMinY) * toMetersFactor;

            rMinX = std::min(rMinX, posX); rMaxX = std::max(rMaxX, posX);
            rMinY = std::min(rMinY, posY); rMaxY = std::max(rMaxY, posY);
            j++;
        }

        RawComp c;
        c.label = (tempPoints[i].label == "ONBEKEND") ? (tempPoints[i].type + "_" + std::to_string(currentID)) : tempPoints[i].label;
        c.type = tempPoints[i].type;
        c.id = currentID;
        c.rMinX = rMinX; c.rMaxX = rMaxX; c.rMinY = rMinY; c.rMaxY = rMaxY;
        allComponents.push_back(c);
        i = j;
    }

    // STAP 2: FILTER DE PAGINARAND ERUIT
    std::vector<RawComp> nonPageComponents;
    double documentMaxX = (gMaxX - gMinX) * toMetersFactor;
    double documentMaxY = (gMaxY - gMinY) * toMetersFactor;

    for (const auto& c : allComponents) {
        bool touchesLeft   = (c.rMinX < 0.001);
        bool touchesRight  = (c.rMaxX > documentMaxX - 0.001);
        bool touchesBottom = (c.rMinY < 0.001);
        bool touchesTop    = (c.rMaxY > documentMaxY - 0.001);

        if (touchesLeft && touchesRight && touchesBottom && touchesTop) {
            continue; // Paginarand weggooien
        }
        nonPageComponents.push_back(c);
    }

    if (nonPageComponents.empty()) {
        panelWidthMeters = 0.0; panelHeightMeters = 0.0;
        finalBlocks.clear(); tempPoints.clear();
        return;
    }

    // STAP 3: BEREKEN DE BOUNDING BOX VAN DE OVERGEBLEVEN DELEN (HET PANEEL)
    double compMinX = 1e15;  double compMaxX = -1e15;
    double compMinY = 1e15;  double compMaxY = -1e15;
    for (const auto& c : nonPageComponents) {
        compMinX = std::min(compMinX, c.rMinX);
        compMaxX = std::max(compMaxX, c.rMaxX);
        compMinY = std::min(compMinY, c.rMinY);
        compMaxY = std::max(compMaxY, c.rMaxY);
    }

    // De grootte van de achterplaat sluit nu perfect aan
    panelWidthMeters = compMaxX - compMinX;
    panelHeightMeters = compMaxY - compMinY;

    // STAP 4: TRANSFORMEER NAAR HET ZUIVERE POSITIEVE ASSENSTELSEL & FILTER OP GROOTTE
    finalBlocks.clear();
    for (const auto& c : nonPageComponents) {
        double tMinX = c.rMinX - compMinX;
        double tMaxX = c.rMaxX - compMinX;
        double tMinY = c.rMinY - compMinY;
        double tMaxY = c.rMaxY - compMinY;

        double breedte = tMaxX - tMinX;
        double hoogte = tMaxY - tMinY;

        if (breedte <= maxGrensInMeters && hoogte <= maxGrensInMeters) {
            BlockResult res;
            res.label = c.label;
            res.type = c.type;
            res.minX = tMinX;
            res.maxX = tMaxX;
            res.minY = tMinY;
            res.maxY = tMaxY;
            res.centerX = (tMinX + tMaxX) / 2.0;
            res.centerY = (tMinY + tMaxY) / 2.0;
            finalBlocks.push_back(res);
        }
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
            // Let op: yThreshold moet nu in METERS worden meegegeven (bijv. 0.02 voor 2cm)
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
    std::cout << "\n--- GEVONDEN RAILS (Opgeslagen in METERS voor Robot) ---" << std::endl;
    for (auto const& [id, lijst] : rails) {
        std::cout << "RAIL " << id << " (" << lijst.size() << " componenten):" << std::endl;
        for (const auto& c : lijst) {
            // Print met floating-point precisie
            std::cout << "  -> " << c.type << " op X: " << c.centerX << " m, Y: " << c.centerY << " m" << std::endl;
        }
    }
}
