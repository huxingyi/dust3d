#include <QDebug>
#include "genericspineandpseudophysics.h"
 
const double GenericSpineAndPseudoPhysics::g = 9.80665;

void GenericSpineAndPseudoPhysics::calculateFootHeights(double preferredHeight, double stanceTime, double swingTime, 
        std::vector<double> *heights, std::vector<double> *moveOffsets)
{
    double totalTime = stanceTime + swingTime;
    double halfSwingTime = 0.5 * swingTime;
    
    double pelvisVelocity = g * (halfSwingTime);
    double receptionTime = 0.5 * stanceTime;
    double propulsionTime = stanceTime - receptionTime;
    
    double pelvisAcceleration = pelvisVelocity / receptionTime;
    double frameTime = 1.0 / 60;
    
    double deformHeight = 0;
    for (double time = 0.0; time < totalTime; time += frameTime) {
        double t, v0, a, s;
        if (time < stanceTime) {
            if (time < receptionTime) {
                t = time;
                a = -pelvisAcceleration;
                v0 = pelvisVelocity;
                s = v0 * t + 0.5 * a * std::pow(t, 2);
                qDebug() << "Stance[Reception] s:" << s << "time:" << time << "t:" << t << "v0:" << v0 << "a:" << a;
                deformHeight = s;
                heights->push_back(preferredHeight - s);
                if (nullptr != moveOffsets)
                    moveOffsets->push_back((receptionTime - time) / receptionTime);
            } else {
                t = time - receptionTime;
                a = pelvisAcceleration;
                v0 = 0;
                s = v0 * t + 0.5 * a * std::pow(t, 2);
                qDebug() << "Stance[Propulsion] s:" << s << "time:" << time << "t:" << t << "v0:" << v0 << "a:" << a;
                heights->push_back(preferredHeight - (deformHeight - s));
                if (nullptr != moveOffsets)
                    moveOffsets->push_back(-t / propulsionTime);
            }
        } else {
            if (time - stanceTime < halfSwingTime) {
                t = time - stanceTime;
                a = -g;
                v0 = pelvisVelocity;
                s = v0 * t + 0.5 * a * std::pow(t, 2);
                qDebug() << "Swing[1/2] s:" << s << "time:" << time << "t:" << t << "v0:" << v0 << "a:" << a;
                deformHeight = s;
                heights->push_back(preferredHeight + s);
                if (nullptr != moveOffsets)
                    moveOffsets->push_back(-(halfSwingTime - t) / halfSwingTime);
            } else {
                t = time - stanceTime - halfSwingTime;
                a = g;
                v0 = 0;
                s = v0 * t + 0.5 * a * std::pow(t, 2);
                qDebug() << "Swing[2/2] s:" << s << "time:" << time << "t:" << t << "v0:" << v0 << "a:" << a;
                heights->push_back(preferredHeight + (deformHeight - s));
                if (nullptr != moveOffsets)
                    moveOffsets->push_back(t / halfSwingTime);
            }
        }
    }
    
    double time = 0.0;
    for (size_t i = 0; i < heights->size(); ++i) {
        QString phase;
        if (time <= stanceTime) {
            if (time <= receptionTime) {
                phase = "Stance[Reception]";
            } else {
                phase = "Stance[Propulsion]";
            }
        } else {
            if (time - stanceTime < halfSwingTime) {
                phase = "Swing[1/2]";
            } else {
                phase = "Swing[2/2]";
            }
        }
        qDebug() << phase << "frame[" << i << "]:" << (*heights)[i] << " move:" << (*moveOffsets)[i];
        time += frameTime;
    }
}

