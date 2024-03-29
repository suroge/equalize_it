#include "filter_response.hpp"

#include "constants.hpp"

FilterFrequencyResponse::FilterFrequencyResponse(PluginProcessor &p, int id)
    : pluginProcessor(p), filterID(id), colour(colours::getFilterColour(id)),
      parameters(id, p.getAPVTS()),
      frequencyResponse([](float) { return 0.0f; }) {
  startTimerHz(24);
}

void FilterFrequencyResponse::paint(juce::Graphics &g) {
  if (pluginProcessor.getSampleRate() == 0) {
    return;
  }

  const float xMin = 0.0f;
  float xMax = static_cast<float>(getWidth());
  const float yMax = static_cast<float>(getHeight());

  const float halfSampleRate =
      static_cast<float>(pluginProcessor.getSampleRate() * 0.5);

  const auto xToFreq = math::invLogMapping(xMin, xMax, constants::GRID_MIN_FREQ,
                                           constants::GRID_MAX_FREQ);
  const auto dBToY = math::segmentMapping(constants::GRID_MIN_DB,
                                          constants::GRID_MAX_DB, yMax, 0.0f);

  juce::Path freqRespPath;
  freqRespPath.preallocateSpace(3 * (static_cast<int>(xMax) + 3));

  float xLast = xMin;
  float yLast = dBToY(frequencyResponse(xToFreq(xMin)));
  const float yStart = yLast;
  freqRespPath.startNewSubPath(xLast, yLast);
  for (float x = xMin + 1; x < xMax; ++x) {
    const float freq = xToFreq(x);
    if (freq > halfSampleRate) {
      break;
    }
    xLast = x;
    yLast = dBToY(frequencyResponse(freq));
    freqRespPath.lineTo(xLast, yLast);
  }

  g.setColour(colour);
  g.strokePath(freqRespPath, juce::PathStrokeType(
                                 1.f, juce::PathStrokeType::JointStyle::curved,
                                 juce::PathStrokeType::EndCapStyle::rounded));

  if (parameters.getIsActiveValue()) {
    freqRespPath.lineTo(xLast, juce::jmax(yStart, yLast));
    freqRespPath.lineTo(xMin, juce::jmax(yStart, yLast));
    freqRespPath.closeSubPath();

    if (pluginProcessor.getUiState().selectedFilterID == filterID) {
      g.setColour(colour.withAlpha(0.1f));
    } else {
      g.setColour(colour.withAlpha(0.01f));
    }
    g.fillPath(freqRespPath);
  }
}

void FilterFrequencyResponse::timerCallback() {
  const auto equalizerProcessor = pluginProcessor.getEqualizerProcessor();
  if (!equalizerProcessor) {
    return;
  }

  const auto filterProcessor = equalizerProcessor->getFilter(filterID);
  if (!filterProcessor) {
    return;
  }

  frequencyResponse = filterProcessor->getFrequencyResponse();
}

std::function<float(float)> &FilterFrequencyResponse::getFrequencyResponse() {
  return frequencyResponse;
}