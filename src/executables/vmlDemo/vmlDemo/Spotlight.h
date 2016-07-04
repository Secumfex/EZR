//
// Created by Windrian on 08.01.2016.
//

#ifndef EZR_SPOTLIGHT_H
#define EZR_SPOTLIGHT_H

#include <glm/glm.hpp>

class Spotlight {

public:
    Spotlight(const glm::vec4& color, float phi);

private:

    glm::vec4 m_color;
    float m_phi;            // radiant flux


};


#endif //EZR_SPOTLIGHT_H
