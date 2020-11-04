#pragma once

#include <utils.hpp>

#include <random>
#include <utility>


class Optimizer {
public:
    /// \brief Constructor 
    Optimizer(int maskSize, int dimension);

    /// \brief Free the GL ressources before the destruction of the object.
    /// \note This is required because otherwise the context will be destroyed before the ressources are freed.
    void freeGLRessources();

    /// \brief Dispatch the compute shader.
    void run() const;

    /// \brief Query the number of permutations that was accepted in the last compute shader dispatch.
    /// \return The number of permutations that was accepted in the last dispatch.
    int acceptedSwapCount() const;

    /// \brief Accessor for the mask OpenGL texture.
    /// \return The OpenGL texture ID of the mask.
    GLuint maskTexture() const { return m_maskIn; }

    /// \brief Export the latest mask as a .ppm image.
    /// \param filename The name of the file to export the mask in.
    void exportMaskAsPPM(const char *filename) const;

    /// \brief Export the latest mask as a header.
    /// \param filename The name of the file to export the mask in.
    void exportMaskAsHeader(const char *filename) const;

private:
    GLuint m_program;

    GLuint m_maskIn;

    GLuint m_maskOut;

    GLuint m_atomicCounter;

    const int m_workGroupCount;

    mutable int m_layers;

    const int m_maskSize;

    const int m_dimension;

    mutable std::mt19937 m_generator;

    GLuint m_permutationsSSBO;


    //// Refactoring functions ////

    /// \brief Generate the permutations that will be tested by the compute shader and store them in an SSBO.
    void generatePermutationsSSBO();

    /// \brief Generate the atomic coutner used to track the number of swaps in a single dispatch.
    void generateAtomicCounter();

    /// \brief Create the white noise textures and send them to the GPU.
    void setupMaskTextures();

    /// \brief Generate an OpenGL RGBA32F 3D texture.
    /// \param data The data to store in the texture (i.e. the white noise).
    /// \return The OpenGL texture ID.
    GLuint generateMaskTexture(const void *data);
};
