#pragma once

#include <vk_types.h>
#include <vector>
#include <functional>
#include <string>

// Forward declarations
class VulkanEngine;

namespace Yalaz::Renderer {

// =============================================================================
// POST-PROCESS PASS - Base class for all post-processing effects
// =============================================================================
class PostProcessPass {
public:
    PostProcessPass(VulkanEngine* engine, const std::string& name);
    virtual ~PostProcessPass() = default;

    // Initialize GPU resources (pipelines, descriptors)
    virtual void init() = 0;

    // Cleanup GPU resources
    virtual void cleanup() = 0;

    // Execute the pass
    virtual void execute(VkCommandBuffer cmd, AllocatedImage& input, AllocatedImage& output) = 0;

    // Resize handling
    virtual void onResize(VkExtent2D newExtent) {}

    // Pass control
    bool isEnabled() const { return _enabled; }
    void setEnabled(bool enabled) { _enabled = enabled; }
    const std::string& getName() const { return _name; }

protected:
    VulkanEngine* _engine;
    std::string _name;
    bool _enabled = true;
};

// =============================================================================
// POST-PROCESS MANAGER - Orchestrates post-processing pipeline
// =============================================================================
class PostProcessManager {
public:
    PostProcessManager(VulkanEngine* engine);
    ~PostProcessManager();

    // Initialize the post-process system
    void init();

    // Cleanup all resources
    void cleanup();

    // Execute all enabled passes
    void execute(VkCommandBuffer cmd, AllocatedImage& sceneColor, AllocatedImage& finalOutput);

    // Resize all passes
    void onResize(VkExtent2D newExtent);

    // Pass management
    void addPass(std::unique_ptr<PostProcessPass> pass);
    PostProcessPass* getPass(const std::string& name);

    template<typename T>
    T* getPass() {
        for (auto& pass : _passes) {
            if (T* typed = dynamic_cast<T*>(pass.get())) {
                return typed;
            }
        }
        return nullptr;
    }

    // Global post-process settings
    struct Settings {
        bool enabled = true;
        float exposure = 1.0f;
        float gamma = 2.2f;
    };
    Settings settings;

    // Ping-pong buffers for multi-pass processing
    AllocatedImage& getPingBuffer() { return _pingBuffer; }
    AllocatedImage& getPongBuffer() { return _pongBuffer; }
    void swapPingPong() { _currentIsPing = !_currentIsPing; }
    AllocatedImage& getCurrentBuffer() { return _currentIsPing ? _pingBuffer : _pongBuffer; }
    AllocatedImage& getNextBuffer() { return _currentIsPing ? _pongBuffer : _pingBuffer; }

private:
    VulkanEngine* _engine;
    std::vector<std::unique_ptr<PostProcessPass>> _passes;

    // Ping-pong buffers for effect chaining
    AllocatedImage _pingBuffer;
    AllocatedImage _pongBuffer;
    bool _currentIsPing = true;

    VkExtent2D _extent;

    void createPingPongBuffers();
    void destroyPingPongBuffers();
};

// =============================================================================
// RENDER SETTINGS - Global rendering quality settings
// =============================================================================
struct RenderSettings {
    // === SSAO Settings ===
    bool ssaoEnabled = true;
    int ssaoSamples = 32;
    float ssaoRadius = 0.5f;
    float ssaoIntensity = 1.0f;
    float ssaoBias = 0.025f;
    int ssaoBlurPasses = 2;

    // === Bloom Settings ===
    bool bloomEnabled = true;
    float bloomThreshold = 0.8f;
    float bloomIntensity = 0.5f;
    int bloomMipLevels = 7;
    float bloomRadius = 1.0f;

    // === Tone Mapping Settings ===
    bool tonemappingEnabled = true;
    int tonemapOperator = 0;  // 0=ACES, 1=Reinhard, 2=Uncharted2, 3=Linear
    float exposure = 1.0f;
    float gamma = 2.2f;

    // === Color Grading ===
    float contrast = 1.0f;
    float saturation = 1.0f;
    float sharpness = 0.0f;    // 0..1, post-tonemap micro-contrast sharpening
    float temperature = 0.0f;  // -1 to 1 (cool to warm)
    float tint = 0.0f;         // -1 to 1 (green to magenta)

    // === SSR Settings ===
    bool ssrEnabled = false;
    int ssrMaxSteps = 128;
    float ssrMaxDistance = 100.0f;
    float ssrThickness = 0.5f;
    float ssrRoughnessThreshold = 0.5f;

    // === Shadow Settings ===
    bool pcssEnabled = true;
    int pcssBlockerSamples = 32;
    int pcssPCFSamples = 64;
    float pcssLightSize = 0.02f;
    float pcssMinPenumbra = 0.0f;

    bool contactShadowsEnabled = true;
    int contactShadowSteps = 32;
    float contactShadowLength = 0.5f;
    float contactShadowFadeStart = 0.8f;

    // === Spot Light Settings ===
    bool spotLightsEnabled = true;
    int maxSpotLights = 16;
    bool spotLightShadowsEnabled = true;

    // === Reflection Probe Settings ===
    bool reflectionProbesEnabled = true;
    float globalSkyBlend = 0.3f;
};

} // namespace Yalaz::Renderer
