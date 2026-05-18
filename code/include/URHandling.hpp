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
    
    // Pure, normale objecten garanderen de punt-notatie overal!
    ur_rtde::RTDEControlInterface control;
    ur_rtde::RTDEReceiveInterface receive;

public:
    // De constructor eist het IP-adres (geen lege constructor nodig)
    RobotManager(std::string ip); 
    ~RobotManager();

    void PlaceComponent(const std::vector<double>& PlacePoint);
    void ClickIn();         
    void PickupComponent();  
    std::vector<double> GetRobotPose();
    void MoveToPose(const std::vector<double>& pose);
};

#endif
