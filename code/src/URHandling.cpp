// ==================== URHandling.cpp ====================
#include "../include/URHandling.hpp"
#include <iostream>

// Initialiseer direct via de initializer list
RobotManager::RobotManager(std::string ip) : robotIp(ip), control(ip), receive(ip) {
    std::cout << "RobotManager: RTDE verbindingen succesvol opgebouwd op " << ip << std::endl;
}

RobotManager::~RobotManager() {
    control.stopScript(); // Pure punt-notatie!
}

void RobotManager::PlaceComponent(const std::vector<double>& PlacePoint) {
    std::vector<double> Appr = PlacePoint;
    std::vector<double> Find = PlacePoint;
    std::vector<double> direction = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; 
    double acceleration = 0.5;
    std::vector<double> speed = {0.05, 0.0, 0.0, 0.0, 0.0, 0.0};

    Appr[0] -= 0.05;
    Appr[1] += 0.07;
    Find[0] -= 0.05;
    Find[1] += 0.015;
    
    // OVERAL DE PURE PUNT-NOTATIE ZONDER HAAKJES OF PIJLTJES:
    control.moveL(Appr, 0.2, 0.4, false);
    control.moveL(Find, 0.2, 0.4, false);
    control.moveUntilContact(speed, direction, acceleration);
    
    double radialen = -10.0 * (3.14159 / 180.0);
    std::vector<double> StartPoint = receive.getActualTCPPose();
    std::vector<double> transformatie = {0.0, 0.0, 0.0, 0.0, radialen, 0.0};
    std::vector<double> target_pose = control.poseTrans(StartPoint, transformatie);
    
    target_pose[2] += 0.008;
    target_pose[1] -= 0.028;
    control.moveL(target_pose, 0.2, 0.2);
}

std::vector<double> RobotManager::GetRobotPose() {
    return receive.getActualTCPPose(); // Pure punt-notatie!
}

void RobotManager::MoveToPose(const std::vector<double>& pose) {
    control.moveL(pose, 0.2, 0.2);     // Pure punt-notatie!
}

void RobotManager::ClickIn() {}
void RobotManager::PickupComponent() {}
