#pragma once

#include <vk_types.h>
#include <vector>
#include "vk_engine.h"

//void draw_grid(DrawContext& ctx);
//void init_grid_pipeline(VulkanEngine* engine);

class PipelineBuilder {
    //> pipeline
public:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;

    VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
    std::vector<VkVertexInputBindingDescription> _vertexBindings;
    std::vector<VkVertexInputAttributeDescription> _vertexAttributes;
    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineLayout _pipelineLayout;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;
    VkPipelineRenderingCreateInfo _renderInfo;
    VkFormat _colorAttachmentformat;

    PipelineBuilder() { clear(); }

    void clear();
    void set_pipeline_layout(VkPipelineLayout layout);

    VkPipeline build_pipeline(VkDevice device);
    //< pipeline
    void set_shaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
    void set_input_topology(VkPrimitiveTopology topology);
    void set_polygon_mode(VkPolygonMode mode);
    void set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    void set_multisampling_none();
    void disable_blending();
    void enable_blending_additive();
    void enable_blending_alphablend();

    void set_color_attachment_format(VkFormat format);
    void set_depth_format(VkFormat format);
    void disable_depthtest();
    void enable_depthtest(bool depthWriteEnable, VkCompareOp op);
    void set_vertex_input(const VertexInputDescription& description);
};

namespace vkutil {
    bool load_shader_module(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);
}
