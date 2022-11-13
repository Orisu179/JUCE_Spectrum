#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() : forwardFFT (fftOrder),
                                window (fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (800, 600);
    setAudioChannels (2,0);
    startTimerHz(60);


}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    if(bufferToFill.buffer -> getNumChannels() >0 ){
        auto* channelData = bufferToFill.buffer->getReadPointer(0, bufferToFill.startSample);

        for(auto i{0}; i<bufferToFill.numSamples; ++i){
            pushNextSampleIntoFifo (channelData[i]);
        }
    }
    bufferToFill.clearActiveBufferRegion();
}

void MainComponent::pushNextSampleIntoFifo(float sample) noexcept
{
    if(fifoIndex == fftSize){
        if (!nextFFTBlockReady){
            juce::zeromem (fftData, sizeof(fftData));
            memcpy (fftData, fifo, sizeof(fifo));
            nextFFTBlockReady = true;
        }
        fifoIndex = 0;
    }
    fifo[fifoIndex++] = sample;
}

void MainComponent::drawNextFrameSpectrum()
{
    window.multiplyWithWindowingTable (fftData, fftSize);
    forwardFFT.performFrequencyOnlyForwardTransform (fftData);
    auto mindB = -100.0f;
    auto maxdB = 0.0f;
    for(int i{0}; i<scopeSize; ++i)
    {
        auto skewedProportionX = 1.0f - std::exp (std::log (1.0f - (float) i / (float) scopeSize) * 0.2f);
        auto fftDataIndex = juce::jlimit (0, fftSize / 2, (int) (skewedProportionX * (float) fftSize * 0.5f));
        auto level = juce::jmap (juce::jlimit (mindB, maxdB, juce::Decibels::gainToDecibels (fftData[fftDataIndex])
                                                             - juce::Decibels::gainToDecibels ((float) fftSize)), mindB, maxdB, 0.0f, 1.0f);
        scopeData[i] = level;
    }
}

void MainComponent::drawFrame(juce::Graphics& g)
{
    for(int i {0}; i<scopeSize; ++i)
    {
        auto width = getLocalBounds().getWidth();
        auto height = getLocalBounds().getHeight();
        g.drawLine ({ (float) juce::jmap (i-1, 0, scopeSize -1, 0, width),
                      juce::jmap(scopeData[i-1], 0.0f, 1.0f, (float) height, 0.0f),
                      (float) juce::jmap(i, 0, scopeSize-1, 0, width),
                      juce::jmap(scopeData[i], 0.0f, 1.0f, (float)height, 0.0f)
        });
    }
}

void MainComponent::timerCallback()
{
    if(nextFFTBlockReady)
    {
        drawNextFrameSpectrum();
        nextFFTBlockReady = false;
        repaint();
    }
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    g.setOpacity(1.0f);
    drawFrame(g);
    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}
