#include "app.h"


void renderApplication::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    if (queryEnabled)
    {
        vkCmdResetQueryPool(commandBuffer, queryPool, 0, QUERYCOUNT);
        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 0);
        vkCmdResetQueryPool(commandBuffer, pipeStatsQueryPool, 0, 1);
        vkCmdBeginQuery(commandBuffer, pipeStatsQueryPool, 0, 0);
    }

    glm::mat4 projection = MakeInfReversedZProjRH(glm::radians(70.f), float(swapChainExtent.width) / float(swapChainExtent.height), 0.01f);

    {
        if (queryEnabled)
        {
            vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 2);
        }
        glm::mat4 projectionT = glm::transpose(projection);

        DrawCullData cullData = {};
        cullData.frustum[0] = normalizePlane(projectionT[3] + projectionT[0]); // here a frustum plane is defined by p3 + p0 since x / w < -1 <=> x + w < 0 <=> (p3 + p0)*v < 0 <=> a point is outside a plane
        cullData.frustum[1] = normalizePlane(projectionT[3] - projectionT[0]);
        cullData.frustum[2] = normalizePlane(projectionT[3] + projectionT[1]);
        cullData.frustum[3] = normalizePlane(projectionT[3] - projectionT[1]);
        cullData.frustum[4] = normalizePlane(projectionT[3] - projectionT[2]); // watch for reversed-z
        cullData.frustum[5] = glm::vec4(0.f, 0.f, -1.f, drawDistance);
        cullData.drawCount = drawCount;
        cullData.cullingEnabled = cullEnabled;
        cullData.lodEnabled = lodEnabled;
        
        vkCmdFillBuffer(commandBuffer, dccb.buffer, 0, 4, 0);

        VkBufferMemoryBarrier fillBarrier = bufferBarrier(dccb.buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_READ_BIT);
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, 0, 1, &fillBarrier, 0, 0);
          
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, drawcmdPipeline);

        DescriptorInfo descriptors[] = { db.buffer, meshes[0].mb.buffer, dcb.buffer, dccb.buffer };

        vkCmdPushDescriptorSetWithTemplateKHR(commandBuffer, drawcmdProgram.updateTemplate, drawcmdProgram.layout, 0, descriptors);

        vkCmdPushConstants(commandBuffer, drawcmdProgram.layout, drawcmdProgram.pushConstantStages, 0, sizeof(DrawCullData), &cullData);
        vkCmdDispatch(commandBuffer, getGroupCount(uint32_t(draws.size()), drawcullCS.localSizeX), 1, 1);

        VkBufferMemoryBarrier cullBarrier = bufferBarrier(dcb.buffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT);
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 0, 0, 1, &cullBarrier, 0, 0);
        if (queryEnabled)
        {
            vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 3);
        }
    }

    VkImageMemoryBarrier renderBeginBarriers[] =
    {
        imageBarrier(colorTarget.image, 0, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
        imageBarrier(depthTarget.image, 0, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT),
    };
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, sizeof(renderBeginBarriers) / sizeof(renderBeginBarriers[0]), renderBeginBarriers);

    VkClearValue clearValues[2] = {};
    clearValues[0].color = { 0.f, 0.f, 0.f, 1.f };
    clearValues[1].depthStencil = { 0.f, 0 };

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = targetFB;//swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;
    renderPassInfo.clearValueCount = sizeof(clearValues) / sizeof(clearValues[0]);
    renderPassInfo.pClearValues = clearValues;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = (float)swapChainExtent.height;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = -(float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    Globals globals = {};
    globals.projection = projection;

    if (rtxEnabled && rtxSupported)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rtxGraphicsPipeline);

        DescriptorInfo descriptors[] = { dcb.buffer, db.buffer, meshes[0].mlb.buffer, meshes[0].mdb.buffer, meshes[0].vb.buffer };

        vkCmdPushDescriptorSetWithTemplateKHR(commandBuffer, rtxGraphicsProgram.updateTemplate, rtxGraphicsProgram.layout, 0, descriptors);

        vkCmdPushConstants(commandBuffer, rtxGraphicsProgram.layout, rtxGraphicsProgram.pushConstantStages, 0, sizeof(globals), &globals);
        vkCmdDrawMeshTasksIndirectCountNV(commandBuffer, dcb.buffer, offsetof(MeshDrawCommand, indirectMS), dccb.buffer, 0, uint32_t(draws.size()), sizeof(MeshDrawCommand));
    }
    else
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        DescriptorInfo descriptors[] = { dcb.buffer, db.buffer, meshes[0].vb.buffer };

        vkCmdPushDescriptorSetWithTemplateKHR(commandBuffer, graphicsProgram.updateTemplate, graphicsProgram.layout, 0, descriptors);

        // VkBuffer vertexBuffers[] = { meshes[0].vb.buffer };
        VkDeviceSize dummyOffset = 0;
        //vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, &dummyOffset);
        vkCmdBindIndexBuffer(commandBuffer, meshes[0].ib.buffer, dummyOffset, VK_INDEX_TYPE_UINT32);

        vkCmdPushConstants(commandBuffer, graphicsProgram.layout, graphicsProgram.pushConstantStages, 0, sizeof(globals), &globals);
        vkCmdDrawIndexedIndirectCountKHR(commandBuffer, dcb.buffer, offsetof(MeshDrawCommand, indirect), dccb.buffer, 0, uint32_t(draws.size()), sizeof(MeshDrawCommand));
    }

    vkCmdEndRenderPass(commandBuffer);

    VkImageMemoryBarrier depthReadBarriers[] =
    {
        imageBarrier(depthTarget.image, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT),
        imageBarrier(depthPyramid.image, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL),
    };

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, sizeof(depthReadBarriers) / sizeof(depthReadBarriers[0]), depthReadBarriers);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, depthreducePipeline);

    // build depth pyramid
    for (uint32_t i = 0; i < depthPyramidLevels; ++i)
    {
        DescriptorInfo sourceDepth = DescriptorInfo{ depthPyramidMips[i], VK_IMAGE_LAYOUT_GENERAL };// (i == 0) ? DescriptorInfo{ depthTarget.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } : DescriptorInfo{ depthPyramidMips[i - 1], VK_IMAGE_LAYOUT_GENERAL };
        DescriptorInfo descriptors[] = {{ depthPyramidMips[i], VK_IMAGE_LAYOUT_GENERAL }, sourceDepth };

        vkCmdPushDescriptorSetWithTemplateKHR(commandBuffer, depthreduceProgram.updateTemplate, depthreduceProgram.layout, 0, descriptors);

        uint32_t levelWidth = std::max(1u, (swapChainExtent.width / 2) >> i);
        uint32_t levelHeight = std::max(1u, (swapChainExtent.height / 2) >> i);

        vkCmdDispatch(commandBuffer, getGroupCount(levelWidth, depthreduceCS.localSizeX), getGroupCount(levelHeight, depthreduceCS.localSizeY), 1);

        VkImageMemoryBarrier reduceBarrier = imageBarrier(depthPyramid.image, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &reduceBarrier);
    }
    VkImageMemoryBarrier depthWriteBarrier = imageBarrier(depthTarget.image, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &depthWriteBarrier);

    VkRenderPassBeginInfo renderPassLateInfo{};
    renderPassLateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassLateInfo.renderPass = renderPassLate;
    renderPassLateInfo.framebuffer = targetFB;//swapChainFramebuffers[imageIndex];
    renderPassLateInfo.renderArea.offset = { 0, 0 };
    renderPassLateInfo.renderArea.extent = swapChainExtent;

    vkCmdBeginRenderPass(commandBuffer, &renderPassLateInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdEndRenderPass(commandBuffer);

    VkImageMemoryBarrier copyBarriers[] =
    {
        imageBarrier(colorTarget.image, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),
        imageBarrier(swapChainImages[imageIndex], 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    };
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, sizeof(copyBarriers) / sizeof(copyBarriers[0]), copyBarriers);

    if (debugPyramid)
    {
        uint32_t debugLevel = 0;

        VkImageBlit blitRegion = {};
        blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.srcSubresource.mipLevel = debugLevel;
        blitRegion.srcSubresource.layerCount = 1;
        blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.dstSubresource.layerCount = 1;
        blitRegion.srcOffsets[0] = { 0, 0, 0 };
        blitRegion.srcOffsets[1] = { (int32_t)std::max(1u, (swapChainExtent.width / 2) >> debugLevel), (int32_t)std::max(1u, (swapChainExtent.height / 2) >> debugLevel), 1 };
        blitRegion.dstOffsets[0] = { 0, 0, 0 };
        blitRegion.dstOffsets[1] = { (int32_t)swapChainExtent.width, (int32_t)swapChainExtent.height, 1 };

        vkCmdBlitImage(commandBuffer, depthPyramid.image, VK_IMAGE_LAYOUT_GENERAL, swapChainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, VK_FILTER_NEAREST);
    }
    else
    {
        VkImageCopy copyRegion = {};
        copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.srcSubresource.layerCount = 1;
        copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.dstSubresource.layerCount = 1;
        copyRegion.extent = { swapChainExtent.width, swapChainExtent.height, 1 };

        vkCmdCopyImage(commandBuffer, colorTarget.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapChainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
    }

    VkImageMemoryBarrier presentBarrier = imageBarrier(swapChainImages[imageIndex], VK_ACCESS_TRANSFER_WRITE_BIT, 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &presentBarrier);

    if (queryEnabled)
    {
        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, queryPool, 1);
        vkCmdEndQuery(commandBuffer, pipeStatsQueryPool, 0);
    }

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}


void renderApplication::drawFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || !targetFB) {
        recreateSwapChain();

        if (colorTarget.image)
        {
            destroyImage(colorTarget, device);
        }
        if (depthTarget.image)
        {
            destroyImage(depthTarget, device);
        }
        if (targetFB)
        {
            vkDestroyFramebuffer(device, targetFB, 0);
        }

        if (depthPyramid.image)
        {
            for (uint32_t i = 0; i < depthPyramidLevels; ++i)
            {
                vkDestroyImageView(device, depthPyramidMips[i], 0);
            }
            destroyImage(depthPyramid, device);
        }

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        createImage(colorTarget, device, memProperties, swapChainExtent.width, swapChainExtent.height, 1, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        createImage(depthTarget, device, memProperties, swapChainExtent.width, swapChainExtent.height, 1, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        targetFB = createFramebuffer(device, renderPass, colorTarget.imageView, depthTarget.imageView, swapChainExtent.width, swapChainExtent.height);

        depthPyramidLevels = getImageMipLevels(swapChainExtent.width / 2, swapChainExtent.height / 2);

        createImage(depthPyramid, device, memProperties, swapChainExtent.width / 2, swapChainExtent.height / 2, depthPyramidLevels, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

        for (uint32_t i = 0; i < depthPyramidLevels; ++i)
        {
            depthPyramidMips[i] = createImageView(device, depthPyramid.image, VK_FORMAT_R32_SFLOAT, i, 1);
            assert(depthPyramidMips[i]);
        }
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;

        recreateSwapChain();

        if (colorTarget.image)
        {
            destroyImage(colorTarget, device);
        }
        if (depthTarget.image)
        {
            destroyImage(depthTarget, device);
        }
        if (targetFB)
        {
            vkDestroyFramebuffer(device, targetFB, 0);
        }

        if (depthPyramid.image)
        {
            for (uint32_t i = 0; i < depthPyramidLevels; ++i)
            {
                vkDestroyImageView(device, depthPyramidMips[i], 0);
            }
            destroyImage(depthPyramid, device);
        }

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        createImage(colorTarget, device, memProperties, swapChainExtent.width, swapChainExtent.height, 1, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        createImage(depthTarget, device, memProperties, swapChainExtent.width, swapChainExtent.height, 1, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        targetFB = createFramebuffer(device, renderPass, colorTarget.imageView, depthTarget.imageView, swapChainExtent.width, swapChainExtent.height);

        depthPyramidLevels = getImageMipLevels(swapChainExtent.width / 2, swapChainExtent.height / 2);

        createImage(depthPyramid, device, memProperties, swapChainExtent.width / 2, swapChainExtent.height / 2, depthPyramidLevels, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

        for (uint32_t i = 0; i < depthPyramidLevels; ++i)
        {
            depthPyramidMips[i] = createImageView(device, depthPyramid.image, VK_FORMAT_R32_SFLOAT, i, 1);
            assert(depthPyramidMips[i]);
        }
        return;
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    if (queryEnabled)
    {
        vkGetQueryPoolResults(device, queryPool, 0,
            sizeof(queryResults) / sizeof(queryResults[0]), sizeof(queryResults), queryResults, sizeof(queryResults[0]), VK_QUERY_RESULT_WAIT_BIT | VK_QUERY_RESULT_64_BIT);
        vkGetQueryPoolResults(device, pipeStatsQueryPool, 0,
            1, sizeof(pipeStatsQueryResults), pipeStatsQueryResults, sizeof(pipeStatsQueryResults[0]), VK_QUERY_RESULT_WAIT_BIT);

        triangleCount = pipeStatsQueryResults[0];
        frameGPUBegin = double(queryResults[0]) * timestampPeriod * 1e-6;
        frameGPUEnd = double(queryResults[1]) * timestampPeriod * 1e-6;
        frameGPUAvg = frameGPUAvg * 0.95 + (frameGPUEnd - frameGPUBegin) * 0.05;
        cullGPUTime = double(queryResults[3] - queryResults[2]) * timestampPeriod * 1e-6;
    }
}