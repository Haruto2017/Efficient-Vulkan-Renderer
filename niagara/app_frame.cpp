#include "app.h"


void renderApplication::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    vkCmdResetQueryPool(commandBuffer, queryPool, 0, QUERYCOUNT);
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, queryPool, 0);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;

    VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

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

    std::vector<MeshDraw> draws(drawCount);

    for (uint32_t i = 0; i < drawCount; ++i)
    {
        draws[i].offset[0] = float(i % 10) / 10.f + 0.5f / 10.f;
        draws[i].offset[1] = float(i / 10) / 10.f + 0.5f / 10.f;
        draws[i].scale[0] = 1 / 10.f;
        draws[i].scale[1] = 1 / 10.f;
    }

    if (rtxEnabled && rtxSupported)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rtxGraphicsPipeline);

        DescriptorInfo descriptors[] = {meshes[0].vb.buffer, meshes[0].mb.buffer, meshes[0].mdb.buffer};

        vkCmdPushDescriptorSetWithTemplateKHR(commandBuffer, rtxUpdateTemplate, rtxPipelineLayout, 0, descriptors);
        for (int i = 0; i < drawCount; ++i)
        {
            MeshDraw& draw = draws[i];
            vkCmdPushConstants(commandBuffer, rtxPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(draw), &draw);
            vkCmdDrawMeshTasksNV(commandBuffer, uint32_t(meshes[0].m_meshlets.size() / 32), 0);
        }
    }
    else
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        DescriptorInfo descriptors[] = { meshes[0].vb.buffer };

        vkCmdPushDescriptorSetWithTemplateKHR(commandBuffer, updateTemplate, pipelineLayout, 0, descriptors);

        // VkBuffer vertexBuffers[] = { meshes[0].vb.buffer };
        VkDeviceSize dummyOffset = 0;
        //vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, &dummyOffset);
        vkCmdBindIndexBuffer(commandBuffer, meshes[0].ib.buffer, dummyOffset, VK_INDEX_TYPE_UINT32);
        for (int i = 0; i < drawCount; ++i)
        {
            MeshDraw& draw = draws[i];
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(draw), &draw);
            vkCmdDrawIndexed(commandBuffer, meshes[0].m_indices.size(), 1, 0, 0, 0);
        }
    }

    vkCmdEndRenderPass(commandBuffer);

    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, queryPool, 1);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}


void renderApplication::drawFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
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

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapChain();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    if (vkGetQueryPoolResults(device, queryPool, 0,
        sizeof(queryResults) / sizeof(queryResults[0]), sizeof(queryResults), queryResults, sizeof(queryResults[0]), VK_QUERY_RESULT_64_BIT) != VK_NOT_READY)
    {
        frameGPUBegin = double(queryResults[0]) * timestampPeriod * 1e-6;
        frameGPUEnd = double(queryResults[1]) * timestampPeriod * 1e-6;
        frameGPUAvg = frameGPUAvg * 0.95 + (frameGPUEnd - frameGPUBegin) * 0.05;
    }
}