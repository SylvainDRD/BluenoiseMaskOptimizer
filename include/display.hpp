#pragma once

#include <utils.hpp>


class Display {
public:
    Display(int pixelDimension, GLuint maskTexture);

    void freeGLRessources();

    // Display the first dimensions of the mask
    void draw() const;

private:
    GLuint m_program;

    GLuint m_screenquadVAO;

    void generateScreenquad();
};
