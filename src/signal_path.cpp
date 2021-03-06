#include <signal_path.h>

SignalPath::SignalPath() {
    
}

void SignalPath::init(uint64_t sampleRate, int fftRate, int fftSize, dsp::stream<dsp::complex_t>* input, dsp::complex_t* fftBuffer, void fftHandler(dsp::complex_t*)) {
    this->sampleRate = sampleRate;
    this->fftRate = fftRate;
    this->fftSize = fftSize;

    _demod = DEMOD_FM;

    dcBiasRemover.init(input, 32000);
    dcBiasRemover.bypass = true;
    split.init(&dcBiasRemover.output, 32000);

    fftBlockDec.init(&split.output_a, (sampleRate / fftRate) - fftSize, fftSize);
    fftHandlerSink.init(&fftBlockDec.output, fftBuffer, fftSize, fftHandler);

    mainVFO.init(&split.output_b, sampleRate, 200000, 200000, 0, 32000);

    demod.init(mainVFO.output, 100000, 200000, 800);
    amDemod.init(mainVFO.output, 50);
    ssbDemod.init(mainVFO.output, 6000, 3000, 22);
    
    audioResamp.init(&demod.output, 200000, 48000, 800);
    audio.init(&audioResamp.output, 64);
}

void SignalPath::setSampleRate(float sampleRate) {
    dcBiasRemover.stop();
    split.stop();
    fftBlockDec.stop();
    fftHandlerSink.stop();

    demod.stop();
    amDemod.stop();
    audioResamp.stop();

    int inputBlockSize = sampleRate / 200.0f;

    dcBiasRemover.setBlockSize(inputBlockSize);
    split.setBlockSize(inputBlockSize);
    int skip = (sampleRate / fftRate) - fftSize;
    fftBlockDec.setSkip(skip);
    mainVFO.setInputSampleRate(sampleRate, inputBlockSize);

    // // Reset the modulator and audio systems
    setDemodulator(_demod);

    fftHandlerSink.start();
    fftBlockDec.start();
    split.start();
    dcBiasRemover.start();
}

void SignalPath::setVFOFrequency(long frequency) {
    mainVFO.setOffset(frequency);
}

void SignalPath::setVolume(float volume) {
    audio.setVolume(volume);
}

void SignalPath::setDemodulator(int demId) {
    if (demId < 0 || demId >= _DEMOD_COUNT) {
        return;
    }

    audioResamp.stop();

    // Stop current demodulator
    if (_demod == DEMOD_FM) {
        demod.stop();
    }
    else if (_demod == DEMOD_NFM) {
        demod.stop();
    }
    else if (_demod == DEMOD_AM) {
        amDemod.stop();
    }
    else if (_demod == DEMOD_USB) {
        ssbDemod.stop();
    }
    else if (_demod == DEMOD_LSB) {
        ssbDemod.stop();
    }
    _demod = demId;

    // Set input of the audio resampler
    if (demId == DEMOD_FM) {
        mainVFO.setOutputSampleRate(200000, 200000);
        demod.setBlockSize(mainVFO.getOutputBlockSize());
        demod.setSampleRate(200000);
        demod.setDeviation(100000);
        audioResamp.setInput(&demod.output);
        audioResamp.setInputSampleRate(200000, mainVFO.getOutputBlockSize());
        demod.start();
    }
    if (demId == DEMOD_NFM) {
        mainVFO.setOutputSampleRate(12500, 12500);
        demod.setBlockSize(mainVFO.getOutputBlockSize());
        demod.setSampleRate(12500);
        demod.setDeviation(6250);
        audioResamp.setInput(&demod.output);
        audioResamp.setInputSampleRate(12500, mainVFO.getOutputBlockSize());
        demod.start();
    }
    else if (demId == DEMOD_AM) {
        mainVFO.setOutputSampleRate(12500, 12500);
        amDemod.setBlockSize(mainVFO.getOutputBlockSize());
        audioResamp.setInput(&amDemod.output);
        audioResamp.setInputSampleRate(12500, mainVFO.getOutputBlockSize());
        amDemod.start();
    }
    else if (demId == DEMOD_USB) {
        mainVFO.setOutputSampleRate(6000, 3000);
        ssbDemod.setBlockSize(mainVFO.getOutputBlockSize());
        ssbDemod.setMode(dsp::SSBDemod::MODE_USB);
        audioResamp.setInput(&ssbDemod.output);
        audioResamp.setInputSampleRate(6000, mainVFO.getOutputBlockSize());
        ssbDemod.start();
    }
    else if (demId == DEMOD_LSB) {
        mainVFO.setOutputSampleRate(6000, 3000);
        ssbDemod.setBlockSize(mainVFO.getOutputBlockSize());
        ssbDemod.setMode(dsp::SSBDemod::MODE_LSB);
        audioResamp.setInput(&ssbDemod.output);
        audioResamp.setInputSampleRate(6000, mainVFO.getOutputBlockSize());
        ssbDemod.start();
    }

    audioResamp.start();
}

void SignalPath::start() {
    dcBiasRemover.start();
    split.start();

    fftBlockDec.start();
    fftHandlerSink.start();

    mainVFO.start();
    demod.start();

    audioResamp.start();
    audio.start();
}

void SignalPath::setDCBiasCorrection(bool enabled) {
    dcBiasRemover.bypass = !enabled;
}