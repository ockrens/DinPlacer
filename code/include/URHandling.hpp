// ==================== URHandling.hpp ====================
#ifndef URHANDLING_HPP
#define URHANDLING_HPP

#include <ur_rtde/rtde_control_interface.h>
#include <ur_rtde/rtde_receive_interface.h>
#include <string>
#include <vector>

class RobotManager {
private:
    std::string robotIp;
    ur_rtde::RTDEControlInterface control;
    ur_rtde::RTDEReceiveInterface receive;

public:
    RobotManager(std::string ip); 
    ~RobotManager();

    void PlaceComponent(const std::vector<double>& PlacePoint);
    void ClickIn();         
    void PickupComponent();  
    std::vector<double> GetRobotPose();
    void MoveToPose(const std::vector<double>& pose);
    
    // NIEUW: Vraag de actuele verbindingsstatus op bij de ur_rtde bibliotheek
    bool IsConnected();
};

#endif
