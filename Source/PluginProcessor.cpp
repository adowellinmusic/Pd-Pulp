/*
  ==============================================================================

    This file was auto-generated by the Introjucer!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/
#include "PluginProcessor.h"
#include "MainComponent.h"


//==============================================================================
PureDataAudioProcessor::PureDataAudioProcessor()
{
    for (int i=0; i<10; i++) {
        FloatParameter* p = new FloatParameter (0.5, ("Param " + (String) (i+1)).toStdString());
        parameterList.add(p);
        addParameter(p);
    }
}

PureDataAudioProcessor::~PureDataAudioProcessor()
{
    pd = nullptr;
}

//==============================================================================
void PureDataAudioProcessor::setParameterName(int index, String name)
{
    FloatParameter* p = parameterList.getUnchecked(index -1);
    p->setName(name);
}


const String PureDataAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

const String PureDataAudioProcessor::getInputChannelName (int channelIndex) const
{
    return String (channelIndex + 1);
}

const String PureDataAudioProcessor::getOutputChannelName (int channelIndex) const
{
    return String (channelIndex + 1);
}

bool PureDataAudioProcessor::isInputChannelStereoPair (int index) const
{
    return true;
}

bool PureDataAudioProcessor::isOutputChannelStereoPair (int index) const
{
    return true;
}

bool PureDataAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PureDataAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PureDataAudioProcessor::silenceInProducesSilenceOut() const
{
    return false;
}

double PureDataAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PureDataAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PureDataAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PureDataAudioProcessor::setCurrentProgram (int index)
{
}

const String PureDataAudioProcessor::getProgramName (int index)
{
    return String();
}

void PureDataAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void PureDataAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    reloadPatch(sampleRate);
    
    pos = 0;
}

void PureDataAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    if (pd != nullptr)
    {
        pd->computeAudio (false);
        pd->closePatch (patch);
    }

    pd = nullptr;
    pdInBuffer.free();
    pdOutBuffer.free();
}


void PureDataAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    // In case we have more outputs than inputs, this code clears any output channels that didn't contain input data, (because these aren't guaranteed to be empty - they may contain garbage).
    // I've added this to avoid people getting screaming feedback when they first compile the plugin, but obviously you don't need to this code if your algorithm already fills all the output channels.
    for (int i = getNumInputChannels(); i < getNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    int numChannels = jmin (getNumInputChannels(), getNumOutputChannels());
    int len = buffer.getNumSamples();
    int idx = 0;
    
    for (int i=0; i<parameterList.size(); i++) {
        FloatParameter* parameter = parameterList[i];
        pd->sendFloat(parameter->getName(300).toStdString(), parameter->getValue());
    }
    
    MidiMessage message;
    MidiBuffer::Iterator it (midiMessages);
    int samplePosition = buffer.getNumSamples();
    
    while (it.getNextEvent (message, samplePosition))
    {
        if (message.isNoteOn (true)) {
            pd->sendNoteOn (message.getChannel(), message.getNoteNumber(), message.getVelocity());
        }
        if (message.isNoteOff (true)) {
            pd->sendNoteOn (message.getChannel(), message.getNoteNumber(), 0);
        }
    }
    
    while (len > 0)
    {
        int max = jmin (len, pd->blockSize());
        
        /* interleave audio */
        {
            float* dstBuffer = pdInBuffer.getData();
            for (int i = 0; i < max; ++i)
            {
                for (int channelIndex = 0; channelIndex < numChannels; ++channelIndex)
                    *dstBuffer++ = buffer.getReadPointer(channelIndex) [idx + i];
            }
        }
        
        pd->processFloat (1, pdInBuffer.getData(), pdOutBuffer.getData());
        
        /* write-back */
        {
            const float* srcBuffer = pdOutBuffer.getData();
            for (int i = 0; i < max; ++i)
            {
                for (int channelIndex = 0; channelIndex < numChannels; ++channelIndex)
                    buffer.getWritePointer (channelIndex) [idx + i] = *srcBuffer++;
            }
        }
        
        idx += max;
        len -= max;
    }
}

//==============================================================================
bool PureDataAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* PureDataAudioProcessor::createEditor()
{
    return new MainComponent(*this);
}

//==============================================================================
void PureDataAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void PureDataAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

void PureDataAudioProcessor::reloadPatch (double sampleRate)
{
    pd = new pd::PdBase;
    pd->init (getNumInputChannels(), getNumOutputChannels(), sampleRate);
    
    int numChannels = jmin (getNumInputChannels(), getNumOutputChannels());
    pdInBuffer.calloc (pd->blockSize() * numChannels);
    pdOutBuffer.calloc (pd->blockSize() * numChannels);
    
    patch = pd->openPatch ("sawsynth.pd", "/Users/logsol/Dropbox/Basteleien/Sound/Pd");
    jassert (patch.isValid());
    
    pd->computeAudio (true);
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PureDataAudioProcessor();
}
