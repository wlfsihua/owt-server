/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to the
 * source code ("Material") are owned by Intel Corporation or its suppliers or
 * licensors. Title to the Material remains with Intel Corporation or its suppliers
 * and licensors. The Material contains trade secrets and proprietary and
 * confidential information of Intel or its suppliers and licensors. The Material
 * is protected by worldwide copyright and trade secret laws and treaty provisions.
 * No part of the Material may be used, copied, reproduced, modified, published,
 * uploaded, posted, transmitted, distributed, or disclosed in any way without
 * Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery of
 * the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.
 */

#include "AcmmBroadcastGroup.h"

namespace mcu {

using namespace webrtc;
using namespace woogeen_base;

DEFINE_LOGGER(AcmmBroadcastGroup, "mcu.media.AcmmBroadcastGroup");

AcmmBroadcastGroup::AcmmBroadcastGroup()
    : m_groupId(0)
{
    ELOG_DEBUG("AcmmBroadcastGroup");

    m_outputIds.resize(_MAX_OUTPUT_STREAMS_);
    for (size_t i = 0; i < _MAX_OUTPUT_STREAMS_; ++i)
        m_outputIds[i] = true;
}

AcmmBroadcastGroup::~AcmmBroadcastGroup()
{
    ELOG_DEBUG("~AcmmBroadcastGroup");
}

bool AcmmBroadcastGroup::getFreeOutputId(uint16_t *id)
{
    for (size_t i = 0; i < m_outputIds.size(); ++i) {
        if (m_outputIds[i]) {
            m_outputIds[i] = false;

            *id = i;
            return true;
        }
    }

    ELOG_WARN("No free Id, max output reached(%d)!", _MAX_OUTPUT_STREAMS_);
    return false;
}

bool AcmmBroadcastGroup::addDest(const woogeen_base::FrameFormat format, woogeen_base::FrameDestination* destination)
{
    boost::shared_ptr<AcmmOutput> acmmOutput;

    ELOG_DEBUG("addDest: format(%s), dest(%p)", getFormatStr(format), destination);

    if (m_outputMap.find(format) == m_outputMap.end()) {
        ELOG_DEBUG("New format(%s)", getFormatStr(format));

        uint16_t outputId;
        if(!getFreeOutputId(&outputId)) {
            return false;
        }

        int32_t id = (((int32_t)m_groupId << 16) & 0xffff0000) | outputId;
        m_outputMap[format] = boost::shared_ptr<AcmmOutput>(new AcmmOutput(id));
    }

    acmmOutput = m_outputMap[format];
    if (!acmmOutput->addDest(format, destination)) {
        ELOG_ERROR("Can not add dest!");
        return false;
    }

    m_formatMap[destination] = format;
    return true;
}

void AcmmBroadcastGroup::removeDest(woogeen_base::FrameDestination* destination)
{
    woogeen_base::FrameFormat format;

    ELOG_DEBUG("removeDest: dest(%p)", destination);

    if (m_formatMap.find(destination) == m_formatMap.end()) {
        ELOG_ERROR("Invalid dest(%p)", destination);
        return;
    }

    format = m_formatMap[destination];
    if (m_outputMap.find(format) == m_outputMap.end()) {
        ELOG_ERROR("Invalid format(%s)", getFormatStr(format));
        return;
    }

    m_outputMap[format]->removeDest(destination);
}

int32_t AcmmBroadcastGroup::NeededFrequency()
{
    int32_t neededFreq = 0;

    for (auto& it : m_outputMap) {
        boost::shared_ptr<AcmmOutput> output = it.second;
        int32_t freq = output->NeededFrequency();

        if (neededFreq < freq)
            neededFreq = freq;
    }

    return neededFreq;
}

void AcmmBroadcastGroup::NewMixedAudio(const webrtc::AudioFrame *audioFrame)
{
    ELOG_TRACE("newAudioFrame, frame id(0x%x), sample_rate(%d), channels(%ld), samples_per_channel(%ld), timestamp(%d)",
            audioFrame->id_,
            audioFrame->sample_rate_hz_,
            audioFrame->num_channels_,
            audioFrame->samples_per_channel_,
            audioFrame->timestamp_
            );

    for (auto& it : m_outputMap) {
        boost::shared_ptr<AcmmOutput> output = it.second;
        output->newAudioFrame(audioFrame);
    }
}

} /* namespace mcu */