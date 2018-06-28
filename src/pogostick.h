#ifndef POGO_STICK_H
#define POGO_STICK_H
#include <QVector3D>

enum class PogoStickPhase
{
    Reception,
    Propulsion,
    SwingUp,
    SwingDown
};

class PogoStick
{
public:
    PogoStick();
    void setGroundLocation(float groundLocation);
    void setRestPelvisLocation(float restPelvisLocation);
    void setStancePhaseDuration(float duration);
    void setAcceleration(float acceleration);
    void simulate(float time);
    float currentPelvisLocation();
    float currentFootLocation();
    float currentFootHorizontalOffset();
private:
    void calculateParametersForSimulation();
    float calculateDisplacement(float initialVelocity, float acceleration, float time);
private:
    float m_restBodyHeight = 0;
    float m_halfStancePhaseDuration = 0;
    float m_halfSwingPhaseDuration = 0;
    static float m_gravitationalAcceleration;
    float m_airAcceleration = 0;
    float m_accelerationExcludedGravity = 20;
    float m_stancePhaseHeight = 0;
    float m_swingPhaseHeight = 0;
    float m_velocity = 0;
    float m_pelviosLocation = 0;
    float m_groundLocation = 0;
    float m_restPelvisLocation = 0;
    float m_footHorizontalOffset = 0;
    PogoStickPhase m_currentPhase;
    bool needCalculateParameters = true;
};

#endif
