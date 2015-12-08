#include <Internal/AudioModuleImplementation.hpp>
#include <DoremiEngine/Core/Include/SharedContext.hpp>
#include <iostream>
#include <windows.h>
namespace DoremiEngine
{
    namespace Audio
    {
        AudioModuleImplementation::AudioModuleImplementation(const Core::SharedContext& p_sharedContext)
        : m_sharedContext(p_sharedContext)
        {
        }

        AudioModuleImplementation::~AudioModuleImplementation()
        {
        }
        
        void AudioModuleImplementation::ERRCHECK(const FMOD_RESULT& p_Result)
        {
            if (p_Result != FMOD_OK)
            {
                printf("FMOD error! (%d) %s\n", p_Result, FMOD_ErrorString(p_Result));
                //exit(5);
            }
        }

        void AudioModuleImplementation::Startup()
        {
            m_fmodResult = FMOD::System_Create(&m_fmodSystem);
            ERRCHECK(m_fmodResult);
            m_fmodResult = m_fmodSystem->getVersion(&m_fmodVersion);
            if (m_fmodVersion < FMOD_VERSION)
            {
                printf("Error!  You are using an old version of FMOD %08x.  This program requires %08x\n", m_fmodVersion, FMOD_VERSION);
                return;
            }
            m_fmodResult = m_fmodSystem->init(100, FMOD_INIT_NORMAL, 0);
            ERRCHECK(m_fmodResult);
            m_fmodChannel.push_back(m_background);
            m_fmodChannel.push_back(m_enemy);
            m_fmodChannel.push_back(m_record);
        }

        void AudioModuleImplementation::Shutdown()
        {
            size_t t_soundBufferLength = m_fmodSoundBuffer.size();
            for (size_t i = 0; i < t_soundBufferLength; i++)
            {
                m_fmodResult = m_fmodSoundBuffer[i]->release();
                ERRCHECK(m_fmodResult);
            }
            m_fmodResult = m_fmodSystem->close();
            ERRCHECK(m_fmodResult);
            m_fmodResult = m_fmodSystem->release();
            ERRCHECK(m_fmodResult);
        }

        size_t AudioModuleImplementation::LoadSound(const std::string& p_soundName, float p_minDistance, float p_maxDistance)
        {
            std::string fileLocation = m_sharedContext.GetWorkingDirectory() + p_soundName;
            std::string t_soundName = p_soundName;
            char* t_name = new char;
            for(size_t i = 0; i < m_fmodSoundBuffer.size(); i++)
            {
                m_fmodSoundBuffer[i]->getName(t_name, 250);
                std::string t_string;
                t_string = std::string(t_name);
                size_t derp = t_string.size();
                size_t hej = t_soundName.find_last_of("/", t_string.size());
                t_soundName.erase(0, hej + 1);
                if(t_name == t_soundName)
                {

                    return 0;
                }
                return 0;
            }


            FMOD::Sound* t_fmodSound;
            m_fmodResult = m_fmodSystem->createSound(fileLocation.c_str(), FMOD_3D, 0, &t_fmodSound);
            ERRCHECK(m_fmodResult);
            m_fmodResult = t_fmodSound->set3DMinMaxDistance(p_minDistance * m_distanceFactor, p_maxDistance * m_distanceFactor);
            ERRCHECK(m_fmodResult);
            m_fmodResult = t_fmodSound->setMode(FMOD_LOOP_NORMAL);
            ERRCHECK(m_fmodResult);
            m_fmodSoundBuffer.push_back(t_fmodSound);
            size_t returnVal = m_fmodSoundBuffer.size() - 1;
            return returnVal;
        }

        int AudioModuleImplementation::SetSoundPositionAndVelocity(float p_posx, float p_posy, float p_posz, float p_velx, float p_vely, float p_velz,
                                                                   const size_t& p_channelID)
        {
            FMOD_VECTOR pos = {p_posx * m_distanceFactor, p_posy * m_distanceFactor, p_posz * m_distanceFactor};
            FMOD_VECTOR vel = {p_velx * m_distanceFactor, p_vely * m_distanceFactor, p_velz * m_distanceFactor};
            m_fmodResult = m_fmodChannel[p_channelID]->set3DAttributes(&pos, &vel);
            ERRCHECK(m_fmodResult);
            return 0;
        }

        int AudioModuleImplementation::SetListenerPos(float p_posx, float p_posy, float p_posz, float p_forwardx, float p_forwardy, float p_forwardz,
            float p_upx, float p_upy, float p_upz)
        {
            FMOD_VECTOR forward = { p_forwardx, p_forwardy, p_forwardz };
            FMOD_VECTOR up = { p_upx, p_upy, p_upz };
            FMOD_VECTOR listenerPos = { p_posx, p_posy, p_posz };
            
            m_fmodResult = m_fmodSystem->set3DListenerAttributes(0, &listenerPos, 0, &forward, &up);
            ERRCHECK(m_fmodResult);
            return 0;
        }

        void AudioModuleImplementation::PlayASound(size_t p_soundID, bool p_loop, size_t p_channelID)
        {
            if (p_loop)
            {
                m_fmodResult = m_fmodSoundBuffer[p_soundID]->setMode(FMOD_LOOP_NORMAL);
            }
            else
            {
                m_fmodResult = m_fmodSoundBuffer[p_soundID]->setMode(FMOD_LOOP_OFF);
            }
            m_fmodResult = m_fmodSystem->playSound(FMOD_CHANNEL_FREE, m_fmodSoundBuffer[p_soundID], false, &m_fmodChannel[p_channelID]);
            m_fmodChannel[p_channelID]->setVolume(0.5f);
        }

        int AudioModuleImplementation::SetVolumeOnChannel(const size_t& p_channelID, float p_volume)
        {
            m_fmodChannel[p_channelID]->setVolume(p_volume);
            return 0;
        }

        void AudioModuleImplementation::Update()
        {
            m_fmodSystem->update();
            static float derp = 0;
            derp = AnalyseSoundSpectrum(1);
            std::cout << "Freq = " << derp << std::endl;
            static float timer = 0;
            timer+= 0.01f;
            static float posX = 0;
            SetSoundPositionAndVelocity(posX, 0.0f,0.0f , sin(timer), 0.0f, 0.0f, 0);
            posX = sin(timer) * 100;
            if (m_recordingStarted)
            {                
                unsigned int timeElapsedSinceRecordingStarted = 0;
                m_fmodSystem->getRecordPosition(0, &timeElapsedSinceRecordingStarted); /*funkar bara om vi anv�nder outputdriver 0 ...
                kan undvikas genom att l�gga in driverchoice i starten*/
                if (timeElapsedSinceRecordingStarted > 9600)
                {
                    m_fmodResult = m_fmodSystem->playSound(FMOD_CHANNEL_REUSE, m_fmodSoundBuffer[1], false, &m_fmodChannel[1]);
                    ERRCHECK(m_fmodResult);

                    // Dont hear what is being recorded otherwise it will feedback. 
                    //m_fmodResult = m_fmodChannel[1]->setVolume(0);
                    //ERRCHECK(m_fmodResult);
                    m_recordingStarted = false;
                }
            }
        }

        int AudioModuleImplementation::Setup3DSound(float p_dopplerScale, float p_distanceFactor, float p_rollOffScale)
        {
            m_fmodResult = m_fmodSystem->set3DSettings(p_dopplerScale, p_distanceFactor, p_rollOffScale);
            ERRCHECK(m_fmodResult);
            m_distanceFactor = p_distanceFactor;
            return 0;
        }

        size_t AudioModuleImplementation::SetupRecording(bool p_loop)
        {
            FMOD::Sound* t_fmodSound;
            FMOD_CREATESOUNDEXINFO exinfo; 
            memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
            exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
            exinfo.numchannels = 1;
            exinfo.format = FMOD_SOUND_FORMAT_PCM16;
            exinfo.defaultfrequency = 48000;
            exinfo.length = exinfo.defaultfrequency * sizeof(short) * exinfo.numchannels * 5;
            
            if (p_loop)
            {
                m_fmodResult = m_fmodSystem->createSound(0, FMOD_3D | FMOD_SOFTWARE | FMOD_LOOP_NORMAL  | FMOD_OPENUSER, &exinfo, &t_fmodSound);
            }
            else
            {
                m_fmodResult = m_fmodSystem->createSound(0, FMOD_3D | FMOD_SOFTWARE | FMOD_LOOP_OFF | FMOD_OPENUSER, &exinfo, &t_fmodSound);
            }
            ERRCHECK(m_fmodResult);
            m_fmodSoundBuffer.push_back(t_fmodSound);
            size_t r_retVal = m_fmodSoundBuffer.size() - 1;
            return r_retVal;
        }

        int AudioModuleImplementation::StartRecording(size_t p_soundID, bool p_loopRec, size_t p_channelID)
        {
            m_fmodResult = m_fmodSystem->recordStart(0, m_fmodSoundBuffer[p_soundID], p_loopRec);
            ERRCHECK(m_fmodResult);
            /**
                TODOLH Annan l�sning �n sleep
            */
            //Sleep(200); ska ta in det som �r under i updaten
            m_recordingStarted = true;
            //m_fmodResult = m_fmodSystem->playSound(FMOD_CHANNEL_REUSE, m_fmodSoundBuffer[p_soundID], false, &m_fmodChannel[p_channelID]);
            //ERRCHECK(m_fmodResult);

            //// Dont hear what is being recorded otherwise it will feedback. 
            //m_fmodResult = m_fmodChannel[p_channelID]->setVolume(0);
            //ERRCHECK(m_fmodResult);
            return 0;
        }

        float AudioModuleImplementation::AnalyseSoundSpectrum(const size_t& p_channelID)
        {
            FMOD_RESULT t_result;
            float spectrum[m_spectrumSize];
            t_result = m_fmodChannel[p_channelID]->getSpectrum(spectrum, m_spectrumSize, 0, FMOD_DSP_FFT_WINDOW_TRIANGLE);
            ERRCHECK(t_result);

            float max = 0;
            size_t highestFrequencyBand = 0;
            for (size_t i = 0; i < 8192; i++)
            {
                if (spectrum[i]>0.0001f && spectrum[i] > max)
                {
                    max = spectrum[i];
                    highestFrequencyBand = i;
                }
            }
            float dominantHz = (float)highestFrequencyBand * m_binSize;
            /*if (dominantHz < 20)
            {
                dominantHz = 0;
            }*/
            return dominantHz;
        }
    }
}

DoremiEngine::Audio::AudioModule* CreateAudioModule(const DoremiEngine::Core::SharedContext& p_sharedContext)
{
    DoremiEngine::Audio::AudioModule* audio =
    new DoremiEngine::Audio::AudioModuleImplementation(p_sharedContext);
    return audio;
}