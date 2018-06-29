#include <cmath>
#include <QDebug>
#include "pogostick.h"

float PogoStick::m_gravitationalAcceleration = 9.8;

PogoStick::PogoStick()
{
    setStancePhaseDuration(0.95);
}

void PogoStick::setGroundLocation(float groundLocation)
{
    m_groundLocation = groundLocation;
    needCalculateParameters = true;
}

void PogoStick::setRestPelvisLocation(float restPelvisLocation)
{
    m_restPelvisLocation = restPelvisLocation;
    needCalculateParameters = true;
}

void PogoStick::setStancePhaseDuration(float duration)
{
    m_halfStancePhaseDuration = duration * 0.5;
    m_halfSwingPhaseDuration = (1 - duration) * 0.5;
    needCalculateParameters = true;
}

void PogoStick::setAcceleration(float acceleration)
{
    m_accelerationExcludedGravity = acceleration - m_gravitationalAcceleration;
    needCalculateParameters = true;
}

void PogoStick::calculateParametersForSimulation()
{
    m_restBodyHeight = abs(m_restPelvisLocation - m_groundLocation);
    
    float maxAcceleration = m_restBodyHeight / (0.5 * (m_halfStancePhaseDuration * m_halfStancePhaseDuration));
    if (m_accelerationExcludedGravity > maxAcceleration) {
        qDebug() << "Reset acceleration from" << m_accelerationExcludedGravity << "to maxAcceleration:" << maxAcceleration;
        m_accelerationExcludedGravity = maxAcceleration;
    }
    
    m_stancePhaseHeight = calculateDisplacement(0, m_accelerationExcludedGravity, m_halfStancePhaseDuration);
    m_velocity = m_accelerationExcludedGravity * m_halfStancePhaseDuration;
    m_airAcceleration = m_velocity / m_halfSwingPhaseDuration;
    m_swingPhaseHeight = calculateDisplacement(0, m_airAcceleration, m_halfSwingPhaseDuration);
}

float PogoStick::calculateDisplacement(float initialVelocity, float acceleration, float time)
{
    return 0.5 * acceleration * (time * time) + initialVelocity * time;
}

float PogoStick::currentPelvisLocation()
{
    return m_pelviosLocation;
}

float PogoStick::currentFootLocation()
{
    if (PogoStickPhase::Reception == m_currentPhase || PogoStickPhase::Propulsion == m_currentPhase)
        return m_groundLocation;
    return (m_pelviosLocation - m_restBodyHeight);
}

float PogoStick::currentFootHorizontalOffset()
{
    return m_footHorizontalOffset;
}

void PogoStick::simulate(float time)
{
    if (time > 1)
        time -= 1;

    if (needCalculateParameters) {
        needCalculateParameters = false;
        calculateParametersForSimulation();
    }
    
    float stancePhaseDuration = m_halfStancePhaseDuration + m_halfStancePhaseDuration;
    float swingPhaseBegin = stancePhaseDuration;
    float swingPhaseDuration = 1 - stancePhaseDuration;
    if (time <= stancePhaseDuration) {
        if (time <= m_halfStancePhaseDuration) {
            // Reception Phase (down)
            m_currentPhase = PogoStickPhase::Reception;
            float displacement = calculateDisplacement(m_velocity, m_accelerationExcludedGravity, time);
            qDebug() << "Reception Phase (down) displacement:" << displacement;
            m_pelviosLocation = m_restPelvisLocation - displacement;
        } else {
            // Propulsion Phase (up)
            m_currentPhase = PogoStickPhase::Propulsion;
            float displacement = calculateDisplacement(0, m_accelerationExcludedGravity, time - m_halfStancePhaseDuration);
            qDebug() << "Propulsion Phase (up) displacement:" << displacement;
            m_pelviosLocation = m_restPelvisLocation - (m_stancePhaseHeight - displacement);
        }
        m_footHorizontalOffset = -time * 0.5 / stancePhaseDuration;
    } else {
        float swingPhaseMiddle = swingPhaseBegin + (1 - swingPhaseBegin) * 0.5;
        // Swing Phase
        if (time < swingPhaseMiddle) {
            // up
            m_currentPhase = PogoStickPhase::SwingUp;
            float displacement = calculateDisplacement(m_velocity, -m_airAcceleration, time - swingPhaseBegin);
            qDebug() << "up displacement:" << displacement;
            m_pelviosLocation = m_restPelvisLocation + displacement;
        } else {
            // down
            m_currentPhase = PogoStickPhase::SwingDown;
            float displacement = calculateDisplacement(0, m_airAcceleration, time - swingPhaseMiddle);
            qDebug() << "down displacement:" << displacement;
            m_pelviosLocation = m_restPelvisLocation + (m_swingPhaseHeight - displacement);
        }
        m_footHorizontalOffset = (time - swingPhaseBegin) * 0.5 / swingPhaseDuration;
    }
}
