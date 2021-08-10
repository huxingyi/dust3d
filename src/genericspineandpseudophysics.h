#ifndef DUST3D_GENERIC_SPINE_AND_PSEUDO_PHYSICS_H
#define DUST3D_GENERIC_SPINE_AND_PSEUDO_PHYSICS_H
#include <cmath>
#include <vector>
 
class GenericSpineAndPseudoPhysics
{
public:
    GenericSpineAndPseudoPhysics()
    {
    }
    
    static void calculateFootHeights(double preferredHeight, double stanceTime, double swingTime, 
        std::vector<double> *heights, std::vector<double> *moveOffsets=nullptr);
        
private:
    static const double g;
};

#endif
