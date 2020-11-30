#include <optimizer.hpp>


// Swap attempts count = Pixel count / (2 * swapAttemptsDivisor)
const int SwapAttemptsDivisor = 2;
const int InvocationsPerWorkgroup = 32;

Optimizer::Optimizer(int maskSize, int dimension)
    : m_workGroupCount(maskSize * maskSize / (2 * InvocationsPerWorkgroup * SwapAttemptsDivisor)),
      m_layers(dimension / 4), m_maskSize(maskSize), m_dimension(dimension) {
    LOG << "Initializing the optimizer..." << std::endl;
    if(dimension != (m_layers * 4))
        ++m_layers;

    m_program = buildShaders({PROJECT_ROOT "shaders/optimizer.comp"}, {GL_COMPUTE_SHADER},
                             {{"MASK_SIZE", m_maskSize}, {"DIMENSION", m_dimension}, {"LAYERS_COUNT", m_layers}});

    m_generator.seed(std::random_device{}());

    generatePermutationsSSBO();
    generateAtomicCounter();
    setupMaskTextures();
}

void Optimizer::freeGLRessources() {
    glDeleteBuffers(1, &m_permutationsSSBO);
    glDeleteBuffers(1, &m_atomicCounter);
    glDeleteTextures(1, &m_maskIn);
    glDeleteTextures(1, &m_maskOut);
    glDeleteProgram(m_program);
}

void Optimizer::run() const {
    glUseProgram(m_program);

    // Set the scramble value for the random permutations
    std::uniform_int_distribution<uint> distribution(0, m_maskSize - 1);
    glUniform2i(glGetUniformLocation(m_program, "scramble"), distribution(m_generator), distribution(m_generator));

    // Dispatch the compute shader and synchronize before updating the input texture
    glDispatchCompute(m_workGroupCount, 1, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

    glCopyImageSubData(m_maskOut, GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, m_maskIn, GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0,
                       m_maskSize, m_maskSize, m_layers);
}

uint32_t Optimizer::acceptedSwapCount() const {
    uint32_t swapCounter;

    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounter);
    GLuint *ptr = (GLuint *)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT);
    swapCounter = *ptr;
    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

    return swapCounter;
}

void Optimizer::exportMaskAsPPM(const char *filename) const {
    std::vector<GLfloat> mask(4 * m_layers * m_maskSize * m_maskSize);

    glBindTexture(GL_TEXTURE_2D_ARRAY, m_maskIn);
    glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, GL_FLOAT, mask.data());

    std::ofstream file;
    file.open(filename, std::ios_base::binary);
    file << "P6" << '\n';
    file << m_maskSize << ' ' << m_maskSize << '\n';
    file << "255" << '\n';

    for(int i = 0; i < m_maskSize * m_maskSize * 4; i += 4) {
        uint8_t pixel[3];

        if(m_dimension == 1) {
            pixel[0] = uint8_t(mask[i] * 255);
            pixel[1] = uint8_t(mask[i] * 255);
            pixel[2] = uint8_t(mask[i] * 255);
        } else {
            pixel[0] = uint8_t(mask[i] * 255);
            pixel[1] = uint8_t(mask[i + 1] * 255);
            pixel[2] = uint8_t(mask[i + 2] * 255);
        }

        file.write(reinterpret_cast<const char *>(&pixel), 3);
    }
}

void Optimizer::exportMaskAsHeader(const char *filename) const {
    const int pixelCount = m_maskSize * m_maskSize;
    std::vector<GLfloat> mask(4 * m_layers * pixelCount);

    glBindTexture(GL_TEXTURE_2D_ARRAY, m_maskIn);
    glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, GL_FLOAT, mask.data());

    std::ofstream file;
    file.open(filename);

    file << "#pragma once\n\n\n";
    file << "static const float mask[" << m_maskSize << "][" << m_maskSize << "][" << m_dimension << "] = {\n";

    for(int i = 0; i < m_maskSize; ++i) {
        file << "    {";

        for(int j = 0; j < m_maskSize; ++j) {
            file << '{';
            int pixelIndex = 4 * (i * m_maskSize + j);
            for(int d = 0; d < m_dimension; d += 4) {
                int offset = d * pixelCount;

                for(int k = 0; k < 4 && d + k < m_dimension; ++k) {
                    file << std::setprecision(10) << mask[pixelIndex + offset + k];

                    if(d + k != m_dimension - 1)
                        file << ", ";
                }
            }
            file << '}';

            if(j != m_maskSize - 1)
                file << ", ";
        }
        file << '}';

        if(i != m_maskSize - 1)
            file << ',';

        file << '\n';
    }
    file << "};\n";
}

void Optimizer::generatePermutationsSSBO() {
    const int pixelCount = m_maskSize * m_maskSize;
    const int permutationArraySize = pixelCount / SwapAttemptsDivisor;

    std::vector<GLuint> permutations(pixelCount);

    for(int i = 0; i < pixelCount; ++i)
        permutations[i] = i;

    for(int i = 0; i < permutationArraySize; ++i) {
        std::uniform_int_distribution<int> distribution(i, pixelCount - 1);

        std::swap(permutations[i], permutations[distribution(m_generator)]);
    }

    glGenBuffers(1, &m_permutationsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_permutationsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * permutationArraySize, permutations.data(), GL_STATIC_DRAW);

    GLuint blockID = glGetProgramResourceIndex(m_program, GL_SHADER_STORAGE_BLOCK, "SwapData");
    glShaderStorageBlockBinding(m_program, blockID, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_permutationsSSBO);
}

void Optimizer::generateAtomicCounter() {
    glGenBuffers(1, &m_atomicCounter);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounter);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), nullptr, GL_DYNAMIC_READ);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, m_atomicCounter);
}

void Optimizer::setupMaskTextures() {
    LOG << "Filling the mask with whitenoise..." << std::endl;
    std::uniform_real_distribution<float> distribution;

    // The number of channels to fill with zeros in the last texture of the array
    const int paddingDimensions = 4 * m_layers - m_dimension;

    const int pixelCount = m_maskSize * m_maskSize;
    const int size = 4 * m_layers * pixelCount;
    const int lastLayerIndex = 4 * pixelCount * (m_layers - 1);

    std::vector<GLfloat> whitenoise(size, 0.f);

    // Fill the m_layers - 1 first rgba textures in the array with noise
    for(int i = 0; i < lastLayerIndex; ++i)
        whitenoise[i] = distribution(m_generator);

    // Fill the 4 - paddingDimensions first dimensions of the last texture with noise
    for(int i = lastLayerIndex; i < 4 * pixelCount * m_layers; i += 4)
        for(int k = 0; k < 4 - paddingDimensions; ++k)
            whitenoise[i + k] = distribution(m_generator);

    m_maskIn = generateMaskTexture(whitenoise.data());
    glBindImageTexture(0, m_maskIn, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);

    m_maskOut = generateMaskTexture(whitenoise.data());
    glBindImageTexture(1, m_maskOut, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
}

GLuint Optimizer::generateMaskTexture(const void *data) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, m_maskSize, m_maskSize, m_layers, 0, GL_RGBA, GL_FLOAT, data);

    return texture;
}
