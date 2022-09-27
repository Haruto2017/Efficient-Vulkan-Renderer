#include "app.h"


void renderApplication::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    vkCmdResetQueryPool(commandBuffer, queryPool, 0, QUERYCOUNT);
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, queryPool, 0);


    glm::mat4 projection = MakeInfReversedZProjRH(glm::radians(70.f), float(swapChainExtent.width) / float(swapChainExtent.height), 0.01f);

    float drawDistance = 50.f;

    {
        glm::mat4 projectionT = glm::transpose(projection);
        glm::vec4 frustum[6];
        frustum[0] = projectionT[3] + projectionT[0]; // here a frustum plane is defined by p3 + p0 since x / w < -1 <=> x + w < 0 <=> (p3 + p0)*v < 0 <=> a point is outside a plane
        frustum[1] = projectionT[3] - projectionT[0];
        frustum[2] = projectionT[3] + projectionT[1];
        frustum[3] = projectionT[3] - projectionT[1];
        frustum[4] = projectionT[3] - projectionT[2]; // watch for reversed-z
        frustum[5] = glm::vec4(0.f, 0.f, -1.f, drawDistance);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, drawcmdPipeline);

        DescriptorInfo descriptors[] = { db.buffer, dcb.buffer };

        vkCmdPushDescriptorSetWithTemplateKHR(commandBuffer, drawcmdProgram.updateTemplate, drawcmdProgram.layout, 0, descriptors);

        vkCmdPushConstants(commandBuffer, drawcmdProgram.layout, drawcmdProgram.pushConstantStages, 0, sizeof(glm::vec4) * 6, &frustum);
        vkCmdDispatch(commandBuffer, uint32_t((draws.size() + 31) / 32), 1, 1);

        VkBufferMemoryBarrier cmdEndBarrier = bufferBarrier(dcb.buffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT);
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 0, 0, 1, &cmdEndBarrier, 0, 0);
    }

    VkImageMemoryBarrier renderBeginBarriers[] =
    {
        imageBarrier(colorTarget.image, 0, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
        imageBarrier(depthTarget.image, 0, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT),
    };
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, sizeof(renderBeginBarriers) / sizeof(renderBeginBarriers[0]), renderBeginBarriers);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = targetFB;//swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;

    VkClearValue clearValues[2] = {};
    clearValues[0].color = { 0.f, 0.f, 0.f, 1.f };
    clearValues[1].depthStencil = { 0.f, 0 };

    renderPassInfo.clearValueCount = sizeof(clearValues) / sizeof(clearValues[0]);
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

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

    Globals globals = {};
    globals.projection = projection;

    if (rtxEnabled && rtxSupported)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rtxGraphicsPipeline);

        DescriptorInfo descriptors[] = { db.buffer, meshes[0].mb.buffer, meshes[0].mdb.buffer, meshes[0].vb.buffer };

        vkCmdPushDescriptorSetWithTemplateKHR(commandBuffer, rtxGraphicsProgram.updateTemplate, rtxGraphicsProgram.layout, 0, descriptors);

        vkCmdPushConstants(commandBuffer, rtxGraphicsProgram.layout, rtxGraphicsProgram.pushConstantStages, 0, sizeof(globals), &globals);
        vkCmdDrawMeshTasksIndirectNV(commandBuffer, dcb.buffer, offsetof(MeshDrawCommand, indirectMS), uint32_t(draws.size()), sizeof(MeshDrawCommand));
    }
    else
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        DescriptorInfo descriptors[] = { db.buffer, meshes[0].vb.buffer };

        vkCmdPushDescriptorSetWithTemplateKHR(commandBuffer, graphicsProgram.updateTemplate, graphicsProgram.layout, 0, descriptors);

        // VkBuffer vertexBuffers[] = { meshes[0].vb.buffer };
        VkDeviceSize dummyOffset = 0;
        //vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, &dummyOffset);
        vkCmdBindIndexBuffer(commandBuffer, meshes[0].ib.buffer, dummyOffset, VK_INDEX_TYPE_UINT32);

        vkCmdPushConstants(commandBuffer, graphicsProgram.layout, graphicsProgram.pushConstantStages, 0, sizeof(globals), &globals);
        vkCmdDrawIndexedIndirect(commandBuffer, dcb.buffer, offsetof(MeshDrawCommand, indirect), uint32_t(draws.size()), sizeof(MeshDrawCommand));
    }

    vkCmdEndRenderPass(commandBuffer);

    VkImageMemoryBarrier copyBarriers[] =
    {
        imageBarrier(colorTarget.image, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),
        imageBarrier(swapChainImages[imageIndex], 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    };
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, sizeof(copyBarriers) / sizeof(copyBarriers[0]), copyBarriers);

    VkImageCopy copyRegion = {};
    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.layerCount = 1;
    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.dstSubresource.layerCount = 1;
    copyRegion.extent = { swapChainExtent.width, swapChainExtent.height, 1 };

    vkCmdCopyImage(commandBuffer, colorTarget.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapChainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    VkImageMemoryBarrier presentBarrier = imageBarrier(swapChainImages[imageIndex], VK_ACCESS_TRANSFER_WRITE_BIT, 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &presentBarrier);

    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, queryPool, 1);

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

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        createImage(colorTarget, device, memProperties, swapChainExtent.width, swapChainExtent.height, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        createImage(depthTarget, device, memProperties, swapChainExtent.width, swapChainExtent.height, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

        targetFB = createFramebuffer(device, renderPass, colorTarget.imageView, depthTarget.imageView, swapChainExtent.width, swapChainExtent.height);
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

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        createImage(colorTarget, device, memProperties, swapChainExtent.width, swapChainExtent.height, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        createImage(depthTarget, device, memProperties, swapChainExtent.width, swapChainExtent.height, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

        targetFB = createFramebuffer(device, renderPass, colorTarget.imageView, depthTarget.imageView, swapChainExtent.width, swapChainExtent.height);
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    if (vkGetQueryPoolResults(device, queryPool, 0,
        sizeof(queryResults) / sizeof(queryResults[0]), sizeof(queryResults), queryResults, sizeof(queryResults[0]), VK_QUERY_RESULT_64_BIT) != VK_NOT_READY)
    {
        frameGPUBegin = double(queryResults[0]) * timestampPeriod * 1e-6;
        frameGPUEnd = double(queryResults[1]) * timestampPeriod * 1e-6;
        frameGPUAvg = frameGPUAvg * 0.95 + (frameGPUEnd - frameGPUBegin) * 0.05;
    }
}